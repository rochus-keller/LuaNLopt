/* Lua adapter to NLopt 2.3
 *
 * Issue: 2012-10-13
 * Status: Alpha
 *
 * Copyright (c) 2012 Rochus Keller, rkeller@nmr.ch
 * Licensed under the GNU Lesser General Public License (LGPL).  
 * See also the COPYRIGHT and README file for details.
 */

#include <Lua/lua.h>
#include <Lua/lauxlib.h>
#include <NLopt/nlopt.h>
#include <vector>

#define LIBNAME		"nlopt"
#define LIBVERSION	LIBNAME " library for " LUA_VERSION
static 	const char* nlopt_metaName = "nlopt_opt";

static void setfieldint( lua_State *L, const char* key, int val )
{
	// Expects table on top of stack
	lua_pushinteger( L, val );
	lua_setfield( L, -2, key );
}

static int algorithm_name( lua_State *L )
{
	const lua_Integer i = luaL_checkinteger( L, 1 );
	if( i < 0 || i >= NLOPT_NUM_ALGORITHMS )
		luaL_argerror( L, 1, "expecting nlopt.algorithm" );
	lua_pushstring( L, nlopt_algorithm_name( static_cast<nlopt_algorithm>( i ) ) );
	return 1;
}

static int srand( lua_State *L )
{
	const lua_Integer i = luaL_checkinteger( L, 1 );
	if( i < 0 )
		luaL_argerror( L, 1, "expecting unsigned long" );
	nlopt_srand( unsigned long( i ) );
	return 0;
}

static int srand_time( lua_State *L )
{
	nlopt_srand_time();
	return 0;
}

static int version( lua_State *L )
{
	int major, minor, bugfix;
	nlopt_version( &major, &minor, &bugfix);
	lua_pushinteger( L, major );
	lua_pushinteger( L, minor );
	lua_pushinteger( L, bugfix );
	return 3;
}

struct nlopt_opt_holder
{
	nlopt_opt d_obj;
};

static void* munge_on_destroy( void* f_data );
static void* munge_on_copy( void* f_data );

static int create( lua_State *L )
{
	const lua_Integer algorithm = luaL_checkinteger( L, 1 );
	if( algorithm < 0 || algorithm >= NLOPT_NUM_ALGORITHMS )
		luaL_argerror( L, 1, "expecting nlopt.algorithm" );
	const lua_Integer n = luaL_checkinteger( L, 2 );
	if( n < 0 )
		luaL_argerror( L, 2, "expecting unsigned integer" );
	nlopt_opt obj = nlopt_create( static_cast<nlopt_algorithm>( algorithm ), (unsigned int) n );
	if( obj == NULL )
		luaL_error( L, "nlopt_create out of memory" );

	nlopt_set_munge( obj, munge_on_destroy, munge_on_copy ); 

	nlopt_opt_holder* holder = static_cast<nlopt_opt_holder*>( lua_newuserdata( L, sizeof(nlopt_opt_holder) ) );
	holder->d_obj = obj;

    luaL_getmetatable( L, nlopt_metaName );
	if( !lua_istable(L, -1 ) )
		luaL_error( L, "internal error: no meta table for '%s'", nlopt_metaName );
	lua_setmetatable( L, -2 );
	return 1;
}

static const luaL_Reg Reg[] =
{
	{ "create", create },
	{ "version", version },
	{ "srand_time", srand_time },
	{ "srand", srand },
	{ "algorithm_name", algorithm_name },
	{ NULL,		NULL	}
};

static nlopt_opt_holder* check(lua_State *L, int narg = 1 )
{
	return static_cast<nlopt_opt_holder*>( luaL_checkudata( L, narg, nlopt_metaName ) );
}
   
static int finalize_nlopt_opt_s( lua_State *L )
{
	nlopt_opt_holder* holder = check( L );
	nlopt_destroy( holder->d_obj );
	return 0;
}

static int tostring( lua_State *L )
{
	nlopt_opt_holder* obj = check( L );
	lua_pushfstring( L, "%s %p", nlopt_metaName, obj );
	return 1;
}

static int copy( lua_State *L )
{
	nlopt_opt_holder* rhs = check( L );
	nlopt_opt obj = nlopt_copy( rhs->d_obj );
	if( obj == NULL )
		luaL_error( L, "nlopt_copy out of memory" );

	nlopt_opt_holder* lhs = static_cast<nlopt_opt_holder*>( lua_newuserdata( L, sizeof(nlopt_opt_holder) ) );
	lhs->d_obj = obj;

    luaL_getmetatable( L, nlopt_metaName );
	if( !lua_istable(L, -1 ) )
		luaL_error( L, "internal error: no meta table for '%s'", nlopt_metaName );
	lua_setmetatable( L, -2 );
	return 1;
}

static int get_algorithm( lua_State *L )
{
	nlopt_opt_holder* holder = check( L );
	lua_pushinteger( L, nlopt_get_algorithm( holder->d_obj ) );
	return 1;
}

static int get_dimension( lua_State *L )
{
	nlopt_opt_holder* holder = check( L );
	lua_pushinteger( L, nlopt_get_dimension( holder->d_obj ) );
	return 1;
}

static int set_lower_bounds( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TTABLE );
	const int n = nlopt_get_dimension( holder->d_obj );
	std::vector<double> lb( n );
	for( int i = 0; i < n; i++ )
	{
		lua_pushinteger( L, i + 1 );
		lua_gettable( L, 2 );
		lb[i] = lua_tonumber( L, -1 );
		lua_pop( L, 1 );
	}
	lua_pushinteger( L, nlopt_set_lower_bounds( holder->d_obj, &lb[0] ) );
	return 1;
}

static int set_upper_bounds( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TTABLE );
	const int n = nlopt_get_dimension( holder->d_obj );
	std::vector<double> lb( n );
	for( int i = 0; i < n; i++ )
	{
		lua_pushinteger( L, i + 1 );
		lua_gettable( L, 2 );
		lb[i] = lua_tonumber( L, -1 );
		lua_pop( L, 1 );
	}
	lua_pushinteger( L, nlopt_set_upper_bounds( holder->d_obj, &lb[0] ) );
	return 1;
}

static int set_lower_bounds1( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double lb = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_lower_bounds1( holder->d_obj, lb ) );
	return 1;
}

static int set_upper_bounds1( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double lb = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_upper_bounds1( holder->d_obj, lb ) );
	return 1;
}

static int get_lower_bounds( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const int n = nlopt_get_dimension( holder->d_obj );
	std::vector<double> lb( n );
	lua_pushinteger( L, nlopt_get_lower_bounds( holder->d_obj, &lb[0] ) );
	lua_createtable( L, n, 0 );
	for( int i = 0; i < n; i++ )
	{
		lua_pushnumber( L, lb[i] );
		lua_rawseti( L, -2, i + 1 );
	}
	return 2;
}

static int get_upper_bounds( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const int n = nlopt_get_dimension( holder->d_obj );
	std::vector<double> lb( n );
	lua_pushinteger( L, nlopt_get_upper_bounds( holder->d_obj, &lb[0] ) );
	lua_createtable( L, n, 0 );
	for( int i = 0; i < n; i++ )
	{
		lua_pushnumber( L, lb[i] );
		lua_rawseti( L, -2, i + 1 );
	}
	return 2;
}

struct callback_context
{
	lua_State *L;
	int ref;
};

static double func(unsigned n, const double* x, double* grad, void* f_data)
{
	// x points to an array of length n
	// if the argument grad is not NULL, then grad points to an array of length n
	// f_data points to a callback_context

	callback_context* ctx = static_cast<callback_context*>( f_data );
	if( ctx )
	{
		lua_rawgeti( ctx->L, LUA_REGISTRYINDEX, ctx->ref );
		const int t = lua_gettop( ctx->L );

		lua_pushliteral( ctx->L, "f" );
		lua_rawget( ctx->L, t );
		if( !lua_isfunction( ctx->L, -1 ) )
		{
			lua_pop( ctx->L, 2 ); // t, f
			return 0.0; // RISK: Fehler melden?
		}
		lua_pushinteger( ctx->L, n );

		lua_pushliteral( ctx->L, "x" );
		lua_rawget( ctx->L, t );
		if( !lua_istable( ctx->L, -1 ) )
		{
			lua_pop( ctx->L, 1 );
			lua_newtable( ctx->L );
			lua_pushliteral( ctx->L, "x" );
			lua_pushvalue( ctx->L, -2 );
			lua_rawset( ctx->L, t );
		}
		const int xt = lua_gettop( ctx->L );
		unsigned int i;
		for( i = 0; i < n; i++ )
		{
			lua_pushnumber( ctx->L, x[i] );
			lua_rawseti( ctx->L, xt, i + 1 );
		}
		// stack: t, f, n, x
		if( grad )
		{
			lua_pushliteral( ctx->L, "grad" );
			lua_rawget( ctx->L, t );
			if( !lua_istable( ctx->L, -1 ) )
			{
				lua_pop( ctx->L, 1 );
				lua_newtable( ctx->L );
				lua_pushliteral( ctx->L, "grad" );
				lua_pushvalue( ctx->L, -2 );
				lua_rawset( ctx->L, t );
			}
			const int gradt = lua_gettop( ctx->L );
			for( i = 0; i < n; i++ )
			{
				lua_pushnumber( ctx->L, grad[i] );
				lua_rawseti( ctx->L, gradt, i + 1 );
			}
		}else
		{
			lua_pushliteral( ctx->L, "grad" );
			lua_pushnil( ctx->L );
			lua_rawset( ctx->L, t );
			lua_pushnil( ctx->L );
		}
		// stack: t, f, n, x, grad | nil
		lua_pushliteral( ctx->L, "f_data" );
		lua_rawget( ctx->L, t );
		// stack: t, f, n, x, grad | nil, f_data | nil
		if( lua_pcall( ctx->L, 4, 1, 0 ) == 0 )
		{
			// stack: t, res
			const double res = lua_tonumber( ctx->L, -1 );
			lua_pop( ctx->L, 1 );
			// stack: t
			if( grad )
			{
				lua_pushliteral( ctx->L, "grad" );
				lua_rawget( ctx->L, t );
				const int gradt = lua_gettop( ctx->L );
				for( i = 0; i < n; i++ )
				{
					lua_rawgeti( ctx->L, gradt, i + 1 );
					grad[i] = lua_tonumber( ctx->L, -1 );
					lua_pop( ctx->L, 1 );
				}
				lua_pop( ctx->L, 1 );
			}
			lua_pop( ctx->L, 1 );
			return res;
		}else
		{
			// stack: t, msg
			lua_pop( ctx->L, 2 );
			return 0.0; // RISK: Fehler melden?
		}
	}else
		return 0.0; // RISK: Fehler melden?
}

static void* munge_on_destroy( void* f_data )
{
	callback_context* ctx = static_cast<callback_context*>( f_data );
	if( ctx )
	{
		luaL_unref( ctx->L, LUA_REGISTRYINDEX, ctx->ref );
		delete ctx;
	}
	return 0;
}

static void* munge_on_copy( void* f_data )
{
	callback_context* ctx = static_cast<callback_context*>( f_data );
	if( ctx )
	{
		lua_rawgeti( ctx->L, LUA_REGISTRYINDEX, ctx->ref );
		const int source = lua_gettop( ctx->L );
		callback_context* ctx_new = new callback_context;
		ctx_new->L = ctx->L;
		lua_newtable( ctx->L );
		const int t = lua_gettop( ctx->L );
		lua_pushvalue( ctx->L, t );
		ctx_new->ref = luaL_ref( ctx->L, LUA_REGISTRYINDEX );

		lua_pushliteral( ctx->L, "f" );
		lua_pushvalue( ctx->L, -1 );
		lua_rawget( ctx->L, source );
		lua_rawset( ctx->L, t );

		lua_pushliteral( ctx->L, "f_data" );
		lua_pushvalue( ctx->L, -1 );
		lua_rawget( ctx->L, source );
		lua_rawset( ctx->L, t );
	
		lua_pop( ctx->L, 2 ); // source and t
		return ctx_new;
	}else
		return NULL;
}

static int set_min_objective( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TFUNCTION );

	callback_context* ctx = new callback_context;
	ctx->L = L;
	lua_newtable( L );
	const int t = lua_gettop( L );
	lua_pushvalue( L, t );
	ctx->ref = luaL_ref( L, LUA_REGISTRYINDEX );

	lua_pushliteral( L, "f" );
	lua_pushvalue( L, 2 );
	lua_rawset( L, t );

	lua_pushliteral( L, "f_data" );
	lua_pushvalue( L, 3 );
	lua_rawset( L, t );

	lua_pop( L, 1 ); // t

	lua_pushinteger( L, nlopt_set_min_objective( holder->d_obj, func, ctx ) );

	// NOTE: der call ist asynchron; in munge_on_destroy wieder freigegeben.
	return 1;
}

static int set_max_objective( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TFUNCTION );

	callback_context* ctx = new callback_context;
	ctx->L = L;
	lua_newtable( L );
	const int t = lua_gettop( L );
	lua_pushvalue( L, t );
	ctx->ref = luaL_ref( L, LUA_REGISTRYINDEX );

	lua_pushliteral( L, "f" );
	lua_pushvalue( L, 2 );
	lua_rawset( L, t );

	lua_pushliteral( L, "f_data" );
	lua_pushvalue( L, 3 );
	lua_rawset( L, t );

	lua_pop( L, 1 ); // t

	lua_pushinteger( L, nlopt_set_max_objective( holder->d_obj, func, ctx ) );

	// NOTE: der call ist asynchron; in munge_on_destroy wieder freigegeben.
	return 1;
}

static int add_inequality_constraint( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TFUNCTION );

	callback_context* ctx = new callback_context;
	ctx->L = L;
	lua_newtable( L );
	const int t = lua_gettop( L );
	lua_pushvalue( L, t );
	ctx->ref = luaL_ref( L, LUA_REGISTRYINDEX );

	lua_pushliteral( L, "f" );
	lua_pushvalue( L, 2 );
	lua_rawset( L, t );

	lua_pushliteral( L, "f_data" );
	lua_pushvalue( L, 3 );
	lua_rawset( L, t );

	lua_pop( L, 1 ); // t

	lua_pushinteger( L, nlopt_add_inequality_constraint( holder->d_obj, func, ctx, lua_tonumber( L, 4 ) ) );

	return 1;
}

static int add_equality_constraint( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TFUNCTION );

	callback_context* ctx = new callback_context;
	ctx->L = L;
	lua_newtable( L );
	const int t = lua_gettop( L );
	lua_pushvalue( L, t );
	ctx->ref = luaL_ref( L, LUA_REGISTRYINDEX );

	lua_pushliteral( L, "f" );
	lua_pushvalue( L, 2 );
	lua_rawset( L, t );

	lua_pushliteral( L, "f_data" );
	lua_pushvalue( L, 3 );
	lua_rawset( L, t );

	lua_pop( L, 1 ); // t

	lua_pushinteger( L, nlopt_add_equality_constraint( holder->d_obj, func, ctx, lua_tonumber( L, 4 ) ) );

	return 1;
}

static int remove_inequality_constraints( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushinteger( L, nlopt_remove_inequality_constraints( holder->d_obj ) );
	return 1;
}

static int remove_equality_constraints( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushinteger( L, nlopt_remove_equality_constraints( holder->d_obj ) );
	return 1;
}

static void mfunc(unsigned m, double *result, unsigned n, const double* x, double* grad, void* f_data)
{
	// x points to an array of length n
	// result, an array of length m
	// if the argument grad is not NULL, then grad points to an array of length m*n
	// f_data points to a callback_context

	callback_context* ctx = static_cast<callback_context*>( f_data );
	if( ctx )
	{
		lua_rawgeti( ctx->L, LUA_REGISTRYINDEX, ctx->ref );
		const int t = lua_gettop( ctx->L );

		lua_pushliteral( ctx->L, "f" );
		lua_rawget( ctx->L, t );
		if( !lua_isfunction( ctx->L, -1 ) )
		{
			lua_pop( ctx->L, 2 ); // t, f
			return; // RISK: Fehler melden?
		}

		lua_pushinteger( ctx->L, m );

		lua_pushliteral( ctx->L, "result" );
		lua_rawget( ctx->L, t );
		if( !lua_istable( ctx->L, -1 ) )
		{
			lua_pop( ctx->L, 1 );
			lua_newtable( ctx->L );
			lua_pushliteral( ctx->L, "result" );
			lua_pushvalue( ctx->L, -2 );
			lua_rawset( ctx->L, t );
		}
		const int resultt = lua_gettop( ctx->L );
		unsigned int i;
		for( i = 0; i < m; i++ )
		{
			lua_pushnumber( ctx->L, result[i] );
			lua_rawseti( ctx->L, resultt, i + 1 );
		}
		// stack: t, f, m, result

		lua_pushinteger( ctx->L, n );

		lua_pushliteral( ctx->L, "x" );
		lua_rawget( ctx->L, t );
		if( !lua_istable( ctx->L, -1 ) )
		{
			lua_pop( ctx->L, 1 );
			lua_newtable( ctx->L );
			lua_pushliteral( ctx->L, "x" );
			lua_pushvalue( ctx->L, -2 );
			lua_rawset( ctx->L, t );
		}
		const int xt = lua_gettop( ctx->L );
		for( i = 0; i < n; i++ )
		{
			lua_pushnumber( ctx->L, x[i] );
			lua_rawseti( ctx->L, xt, i + 1 );
		}
		// stack: t, f, m, result, n, x
		if( grad )
		{
			lua_pushliteral( ctx->L, "grad" );
			lua_rawget( ctx->L, t );
			if( !lua_istable( ctx->L, -1 ) )
			{
				lua_pop( ctx->L, 1 );
				lua_newtable( ctx->L );
				lua_pushliteral( ctx->L, "grad" );
				lua_pushvalue( ctx->L, -2 );
				lua_rawset( ctx->L, t );
			}
			const int gradt = lua_gettop( ctx->L );
			for( i = 0; i < ( n * m ); i++ )
			{
				lua_pushnumber( ctx->L, grad[i] );
				lua_rawseti( ctx->L, gradt, i + 1 );
			}
		}else
		{
			lua_pushliteral( ctx->L, "grad" );
			lua_pushnil( ctx->L );
			lua_rawset( ctx->L, t );
			lua_pushnil( ctx->L );
		}
		// stack: t, f, m, result, n, x, grad | nil
		lua_pushliteral( ctx->L, "f_data" );
		lua_rawget( ctx->L, t );
		// stack: t, f, m, result, n, x, grad | nil, f_data | nil
		if( lua_pcall( ctx->L, 6, 0, 0 ) == 0 )
		{
			// stack: t
			lua_pushliteral( ctx->L, "result" );
			lua_rawget( ctx->L, t );
			const int resultt = lua_gettop( ctx->L );
			for( i = 0; i < m; i++ )
			{
				lua_rawgeti( ctx->L, resultt, i + 1 );
				result[i] = lua_tonumber( ctx->L, -1 );
				lua_pop( ctx->L, 1 );
			}
			if( grad )
			{
				lua_pushliteral( ctx->L, "grad" );
				lua_rawget( ctx->L, t );
				const int gradt = lua_gettop( ctx->L );
				for( i = 0; i < n; i++ )
				{
					lua_rawgeti( ctx->L, gradt, i + 1 );
					grad[i] = lua_tonumber( ctx->L, -1 );
					lua_pop( ctx->L, 1 );
				}
				lua_pop( ctx->L, 1 );
			}

			lua_pop( ctx->L, 1 );
			return;
		}else
		{
			// stack: t, msg
			lua_pop( ctx->L, 2 );
			return; // RISK: Fehler melden?
		}
	}else
		return; // RISK: Fehler melden?
}

static int add_inequality_mconstraint( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const int m = (const int)luaL_checkinteger( L, 2 );
	luaL_checktype( L, 3, LUA_TFUNCTION );
	if( !lua_isnil( L, 5 ) && lua_istable( L, 5 ) )
		luaL_argerror( L, 5, "expecting table or nil" );

	callback_context* ctx = new callback_context;
	ctx->L = L;
	lua_newtable( L );
	const int t = lua_gettop( L );
	lua_pushvalue( L, t );
	ctx->ref = luaL_ref( L, LUA_REGISTRYINDEX );

	lua_pushliteral( L, "f" );
	lua_pushvalue( L, 3 );
	lua_rawset( L, t );

	lua_pushliteral( L, "f_data" );
	lua_pushvalue( L, 4 );
	lua_rawset( L, t );

	lua_pop( L, 1 ); // t

	const double *tol = 0;

	if( lua_isnil( L, 5 ) )
		lua_pushinteger( L, nlopt_add_inequality_mconstraint( holder->d_obj, m, mfunc, ctx, 0 ) );
	else
	{
		std::vector<double> tol( m );
		for( int i = 0; i < m; i++ )
		{
			lua_pushinteger( L, i + 1 );
			lua_gettable( L, 5 );
			tol[i] = lua_tonumber( L, -1 );
			lua_pop( L, 1 );
		}
		lua_pushinteger( L, nlopt_add_inequality_mconstraint( holder->d_obj, lua_tonumber( L, 2 ), mfunc, ctx, &tol[0] ) );
	}

	return 1;
}

static int add_equality_mconstraint( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const int m = (const int)luaL_checkinteger( L, 2 );
	luaL_checktype( L, 3, LUA_TFUNCTION );
	if( !lua_isnil( L, 5 ) && lua_istable( L, 5 ) )
		luaL_argerror( L, 5, "expecting table or nil" );

	callback_context* ctx = new callback_context;
	ctx->L = L;
	lua_newtable( L );
	const int t = lua_gettop( L );
	lua_pushvalue( L, t );
	ctx->ref = luaL_ref( L, LUA_REGISTRYINDEX );

	lua_pushliteral( L, "f" );
	lua_pushvalue( L, 3 );
	lua_rawset( L, t );

	lua_pushliteral( L, "f_data" );
	lua_pushvalue( L, 4 );
	lua_rawset( L, t );

	lua_pop( L, 1 ); // t

	const double *tol = 0;

	if( lua_isnil( L, 5 ) )
		lua_pushinteger( L, nlopt_add_equality_mconstraint( holder->d_obj, m, mfunc, ctx, 0 ) );
	else
	{
		std::vector<double> tol( m );
		for( int i = 0; i < m; i++ )
		{
			lua_pushinteger( L, i + 1 );
			lua_gettable( L, 5 );
			tol[i] = lua_tonumber( L, -1 );
			lua_pop( L, 1 );
		}
		lua_pushinteger( L, nlopt_add_equality_mconstraint( holder->d_obj, lua_tonumber( L, 2 ), mfunc, ctx, &tol[0] ) );
	}

	return 1;
}

static int set_stopval( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double val = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_stopval( holder->d_obj, val ) );
	return 1;
}

static int get_stopval( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushnumber( L, nlopt_get_stopval( holder->d_obj ) );
	return 1;
}

static int set_ftol_rel( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double val = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_ftol_rel( holder->d_obj, val ) );
	return 1;
}

static int get_ftol_rel( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushnumber( L, nlopt_get_ftol_rel( holder->d_obj ) );
	return 1;
}

static int set_ftol_abs( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double val = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_ftol_abs( holder->d_obj, val ) );
	return 1;
}

static int get_ftol_abs( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushnumber( L, nlopt_get_ftol_abs( holder->d_obj ) );
	return 1;
}

static int set_xtol_rel( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double val = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_xtol_rel( holder->d_obj, val ) );
	return 1;
}

static int get_xtol_rel( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushnumber( L, nlopt_get_xtol_rel( holder->d_obj ) );
	return 1;
}

static int set_xtol_abs( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TTABLE );
	const int n = nlopt_get_dimension( holder->d_obj );
	std::vector<double> lb( n );
	for( int i = 0; i < n; i++ )
	{
		lua_pushinteger( L, i + 1 );
		lua_gettable( L, 2 );
		lb[i] = lua_tonumber( L, -1 );
		lua_pop( L, 1 );
	}
	lua_pushinteger( L, nlopt_set_xtol_abs( holder->d_obj, &lb[0] ) );
	return 1;
}

static int get_xtol_abs( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const int n = nlopt_get_dimension( holder->d_obj );
	std::vector<double> lb( n );
	lua_pushinteger( L, nlopt_get_xtol_abs( holder->d_obj, &lb[0] ) );
	lua_createtable( L, n, 0 );
	for( int i = 0; i < n; i++ )
	{
		lua_pushnumber( L, lb[i] );
		lua_rawseti( L, -2, i + 1 );
	}
	return 2;
}

static int set_xtol_abs1( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double tol = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_xtol_abs1( holder->d_obj, tol ) );
	return 1;
}

static int set_maxeval( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const int val = luaL_checkinteger( L, 2 );
	lua_pushinteger( L, nlopt_set_maxeval( holder->d_obj, val ) );
	return 1;
}

static int get_maxeval( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushinteger( L, nlopt_get_maxeval( holder->d_obj ) );
	return 1;
}

static int set_maxtime( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const double val = luaL_checknumber( L, 2 );
	lua_pushinteger( L, nlopt_set_maxtime( holder->d_obj, val ) );
	return 1;
}

static int get_maxtime( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushnumber( L, nlopt_get_maxtime( holder->d_obj ) );
	return 1;
}

static int force_stop( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushnumber( L, nlopt_force_stop( holder->d_obj ) );
	return 1;
}

static int set_force_stop( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	const int val = luaL_checkinteger( L, 2 );
	lua_pushinteger( L, nlopt_set_force_stop( holder->d_obj, val ) );
	return 1;
}

static int get_force_stop( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	lua_pushinteger( L, nlopt_get_force_stop( holder->d_obj ) );
	return 1;
}

static int optimize( lua_State *L )
{
	nlopt_opt_holder* holder = check( L, 1 );
	luaL_checktype( L, 2, LUA_TTABLE );
	const int n = nlopt_get_dimension( holder->d_obj );
	std::vector<double> x( n );
	int i;
	for( i = 0; i < n; i++ )
	{
		lua_pushinteger( L, i + 1 );
		lua_gettable( L, 2 );
		x[i] = lua_tonumber( L, -1 );
		lua_pop( L, 1 );
	}
	double opt_f;
	lua_pushinteger( L, nlopt_optimize( holder->d_obj, &x[0], &opt_f ) );
	lua_pushnumber( L, opt_f );
	for( i = 0; i < n; i++ )
	{
		lua_pushinteger( L, i + 1 );
		lua_pushnumber( L, x[i] );
		lua_settable( L, 2 );
	}
	return 2;
}

static const luaL_Reg Methods[] =
{
	{ "optimize", optimize },
	{ "set_force_stop", set_force_stop },
	{ "get_force_stop", get_force_stop },
	{ "force_stop", force_stop },
	{ "set_maxtime", set_maxtime },
	{ "get_maxtime", get_maxtime },
	{ "set_maxeval", set_maxeval },
	{ "get_maxeval", get_maxeval },
	{ "set_xtol_abs1", set_xtol_abs1 },
	{ "set_xtol_abs", set_xtol_abs },
	{ "get_xtol_abs", get_xtol_abs },
	{ "get_xtol_rel", get_xtol_rel },
	{ "set_xtol_rel", set_xtol_rel },
	{ "get_ftol_abs", get_ftol_abs },
	{ "set_ftol_abs", set_ftol_abs },
	{ "get_ftol_rel", get_ftol_rel },
	{ "set_ftol_rel", set_ftol_rel },
	{ "get_stopval", get_stopval },
	{ "set_stopval", set_stopval },
	{ "add_equality_mconstraint", add_equality_mconstraint },
	{ "add_inequality_mconstraint", add_inequality_mconstraint },
	{ "remove_equality_constraints", remove_equality_constraints },
	{ "remove_inequality_constraints", remove_inequality_constraints },
	{ "add_equality_constraint", add_equality_constraint },
	{ "add_inequality_constraint", add_inequality_constraint },
	{ "set_max_objective", set_max_objective },
	{ "set_min_objective", set_min_objective },
	{ "get_upper_bounds", get_upper_bounds },
	{ "get_lower_bounds", get_lower_bounds },
	{ "set_upper_bounds1", set_upper_bounds1 },
	{ "set_lower_bounds1", set_lower_bounds1 },
	{ "set_upper_bounds", set_upper_bounds },
	{ "set_lower_bounds", set_lower_bounds },
	{ "get_dimension", get_dimension },
	{ "get_algorithm", get_algorithm },
	{ "copy", copy },
	{ NULL,	NULL }
};

static void install_nlopt_opt_s( lua_State *L, const luaL_reg* ms )
{
    const int stackTest = lua_gettop(L);

    if( luaL_newmetatable( L, nlopt_metaName ) == 0 )
		luaL_error( L, "metatable '%s' already registered", nlopt_metaName );

	const int metaTable = lua_gettop(L);

	lua_newtable(L);
	const int methodTable = lua_gettop(L);

    // Mache die methodTable unter dem publicName global zugänglich.
	lua_pushstring(L, nlopt_metaName );
	lua_pushvalue(L, methodTable );
	lua_settable(L, LUA_GLOBALSINDEX ); // TODO: ev. stattdessen als Modul einer Tabelle zuweisen

    // setze Attribut __metatable von meta. Dies ist der Wert, der
	// von getmetatable( obj ) zurückgegeben wird. Trick, um echten Metatable zu verstecken.
    // Wirkt nicht, wenn debug.getmetatable aufgerufen wird.
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodTable );
	lua_settable(L, metaTable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodTable );
	lua_settable(L, metaTable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L , finalize_nlopt_opt_s );
	lua_rawset(L, metaTable);

    lua_pushliteral(L, "__tostring");
	lua_pushcfunction(L, tostring );
	lua_rawset(L, metaTable);

	for( const luaL_reg* l = ms; l && l->name; l++ )
	{
		lua_pushstring(L, l->name);
		lua_pushcfunction(L, l->func );
		lua_rawset(L, methodTable);
	}

	lua_pop(L, 1);  // drop metaTable
	lua_pop(L, 1);  // drop method table
}

#ifdef _WIN32
extern "C"
{
__declspec(dllexport) 
#else
	LUALIB_API 
#endif
int luaopen_LuaNLopt(lua_State *L)
{
    luaL_register( L, LIBNAME, Reg );
    lua_pushliteral( L, "libversion" );			
    lua_pushliteral( L, LIBVERSION );
    lua_settable( L, -3 );

	lua_newtable( L );
	setfieldint( L, "GN_DIRECT", NLOPT_GN_DIRECT );
	setfieldint( L, "GN_DIRECT_L", NLOPT_GN_DIRECT_L );
	setfieldint( L, "GN_DIRECT_L_RAND", NLOPT_GN_DIRECT_L_RAND );
	setfieldint( L, "GN_DIRECT_NOSCAL", NLOPT_GN_DIRECT_NOSCAL );
	setfieldint( L, "GN_DIRECT_L_NOSCAL", NLOPT_GN_DIRECT_L_NOSCAL );
	setfieldint( L, "GN_DIRECT_L_RAND_NOSCAL", NLOPT_GN_DIRECT_L_RAND_NOSCAL );
	setfieldint( L, "GN_ORIG_DIRECT", NLOPT_GN_ORIG_DIRECT );
	setfieldint( L, "GN_ORIG_DIRECT_L", NLOPT_GN_ORIG_DIRECT_L );
	setfieldint( L, "GD_STOGO", NLOPT_GD_STOGO );
	setfieldint( L, "GD_STOGO_RAND", NLOPT_GD_STOGO_RAND );
	setfieldint( L, "LD_LBFGS_NOCEDAL", NLOPT_LD_LBFGS_NOCEDAL );
	setfieldint( L, "LD_LBFGS", NLOPT_LD_LBFGS );
	setfieldint( L, "LN_PRAXIS", NLOPT_LN_PRAXIS );
	setfieldint( L, "LD_VAR1", NLOPT_LD_VAR1 );
	setfieldint( L, "LD_VAR2", NLOPT_LD_VAR2 );
	setfieldint( L, "LD_TNEWTON", NLOPT_LD_TNEWTON );
	setfieldint( L, "LD_TNEWTON_RESTART", NLOPT_LD_TNEWTON_RESTART );
	setfieldint( L, "LD_TNEWTON_PRECOND", NLOPT_LD_TNEWTON_PRECOND );
	setfieldint( L, "LD_TNEWTON_PRECOND_RESTART", NLOPT_LD_TNEWTON_PRECOND_RESTART );
	setfieldint( L, "GN_CRS2_LM", NLOPT_GN_CRS2_LM );
	setfieldint( L, "GN_MLSL", NLOPT_GN_MLSL );
	setfieldint( L, "GD_MLSL", NLOPT_GD_MLSL );
	setfieldint( L, "GN_MLSL_LDS", NLOPT_GN_MLSL_LDS );
	setfieldint( L, "GD_MLSL_LDS", NLOPT_GD_MLSL_LDS );
	setfieldint( L, "LD_MMA", NLOPT_LD_MMA );
	setfieldint( L, "LN_COBYLA", NLOPT_LN_COBYLA );
	setfieldint( L, "LN_NEWUOA", NLOPT_LN_NEWUOA );
	setfieldint( L, "LN_NEWUOA_BOUND", NLOPT_LN_NEWUOA_BOUND );
	setfieldint( L, "LN_NELDERMEAD", NLOPT_LN_NELDERMEAD );
	setfieldint( L, "LN_SBPLX", NLOPT_LN_SBPLX );
	setfieldint( L, "LN_AUGLAG", NLOPT_LN_AUGLAG );
	setfieldint( L, "LD_AUGLAG", NLOPT_LD_AUGLAG );
	setfieldint( L, "LN_AUGLAG_EQ", NLOPT_LN_AUGLAG_EQ );
	setfieldint( L, "LD_AUGLAG_EQ", NLOPT_LD_AUGLAG_EQ );
	setfieldint( L, "LN_BOBYQA", NLOPT_LN_BOBYQA );
	setfieldint( L, "AUGLAG", NLOPT_AUGLAG );
	setfieldint( L, "AUGLAG_EQ", NLOPT_AUGLAG_EQ );
	setfieldint( L, "G_MLSL", NLOPT_G_MLSL );
	setfieldint( L, "G_MLSL_LDS", NLOPT_G_MLSL_LDS );
	setfieldint( L, "LD_SLSQP", NLOPT_LD_SLSQP );
	setfieldint( L, "LD_CCSAQ", NLOPT_LD_CCSAQ );
	setfieldint( L, "NUM_ALGORITHMS", NLOPT_NUM_ALGORITHMS );
	lua_setfield( L, -2, "algorithm" );

	lua_newtable( L );
	setfieldint( L, "FAILURE", NLOPT_FAILURE );
	setfieldint( L, "INVALID_ARGS", NLOPT_INVALID_ARGS );
	setfieldint( L, "OUT_OF_MEMORY", NLOPT_OUT_OF_MEMORY );
	setfieldint( L, "ROUNDOFF_LIMITED", NLOPT_ROUNDOFF_LIMITED );
	setfieldint( L, "FORCED_STOP", NLOPT_FORCED_STOP );
	setfieldint( L, "SUCCESS", NLOPT_SUCCESS );
	setfieldint( L, "STOPVAL_REACHED", NLOPT_STOPVAL_REACHED );
	setfieldint( L, "FTOL_REACHED", NLOPT_FTOL_REACHED );
	setfieldint( L, "XTOL_REACHED", NLOPT_XTOL_REACHED );
	setfieldint( L, "MAXEVAL_REACHED", NLOPT_MAXEVAL_REACHED );
	setfieldint( L, "MAXTIME_REACHED", NLOPT_MAXTIME_REACHED );
	lua_setfield( L, -2, "result" );

	install_nlopt_opt_s( L, Methods );

    return 1;
}
#ifdef _WIN32
}
#endif
