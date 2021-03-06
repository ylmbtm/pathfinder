#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

extern int luaopen_nav_core(lua_State *L);
extern int luaopen_lfs(lua_State *L);

#ifdef FOR_LUA

//#define DTEST
int main(int argc, char* argv[])
{
#ifndef DTEST
	assert(argc == 3);
#endif
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_requiref(L, "nav.core", luaopen_nav_core, 0);
	luaL_requiref(L, "lfs", luaopen_lfs, 0);
#ifndef DTEST
	int status = luaL_loadfile(L, "preprocess.lua");
#else
	int status = luaL_loadfile(L, "loader.lua");
#endif
	if (status != LUA_OK)  {
		fprintf(stderr, "%s\n", lua_tostring(L, -1));
		exit(1);
	}

#ifndef DTEST
	lua_pushstring(L, argv[1]);
	lua_pushstring(L, argv[2]);
	
	status = lua_pcall(L, 2, 0, 0);
#else
	status = lua_pcall(L, 0, 0, 0);
#endif
	if (status != LUA_OK)  {
		fprintf(stderr, "%s\n", lua_tostring(L, -1));
		exit(1);
	}
	lua_close(L);

	return 0;
}

#else

#include "nav.h"
#include <winsock2.h>

double
get_time_millis() {
	struct timeval tv;

#ifdef _WIN32

#define EPOCH_BIAS (116444736000000000)
#define UNITS_PER_SEC (10000000)
#define USEC_PER_SEC (1000000)
#define UNITS_PER_USEC (10)

	union {
		FILETIME ft_ft;
		uint64_t ft_64;
	} ft;

	GetSystemTimeAsFileTime(&ft.ft_ft);

	if ( ft.ft_64 < EPOCH_BIAS ) {
		return -1;
	}
	ft.ft_64 -= EPOCH_BIAS;
	tv.tv_sec = (long)( ft.ft_64 / UNITS_PER_SEC );
	tv.tv_usec = (long)( ( ft.ft_64 / UNITS_PER_USEC ) % USEC_PER_SEC );
#else
	gettimeofday(&tv, NULL);
#endif

	return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}



int main() {
	FILE* fp = fopen("./nav/1005.nav", "rb");

	uint32_t vertex_count;
	fread(&vertex_count, sizeof( uint32_t ), 1, fp);

	double** v_ptr = (double**)malloc(sizeof( *v_ptr ) * vertex_count);
	for ( int i = 0; i < vertex_count; i++ )
	{
		v_ptr[i] = (double*)malloc(sizeof(double)* 3);
		for ( int j = 0; j < 3; j++ )
		{
			float val;
			fread(&val, sizeof( float ), 1, fp);
			v_ptr[i][j] = val;
		}
	}

	uint32_t poly_count;
	fread(&poly_count, sizeof( uint32_t ), 1, fp);

	int** p_ptr = (int**)malloc(sizeof( *p_ptr ) * poly_count);
	for ( int i = 0; i < poly_count; i++ )
	{
		uint8_t index_count;
		fread(&index_count, sizeof( uint8_t ), 1, fp);

		p_ptr[i] = (int*)malloc(sizeof(int)*( index_count + 1 ));
		p_ptr[i][0] = index_count;
		for ( int j = 1; j <= index_count; j++ )
		{
			uint16_t val;
			fread(&val, sizeof( uint16_t ), 1, fp);
			p_ptr[i][j] = val;
		}
	}
	fclose(fp);

	struct nav_mesh_context* mesh = load_mesh(v_ptr, vertex_count, p_ptr, poly_count);

	fp = fopen("./nav/1005.nav.tile", "rb");

	uint32_t tile_unit;
	fread(&tile_unit, sizeof( uint32_t ), 1, fp);

	uint32_t count = 0;
	fread(&count, sizeof( uint32_t ), 1, fp);

	mesh->tile = ( struct nav_tile* )malloc(sizeof( struct nav_tile )*count);
	memset(mesh->tile, 0, sizeof( struct nav_tile )*count);

	uint32_t i;
	for ( i = 0; i < count; i++ ) {

		struct nav_tile* tile = &mesh->tile[i];

		uint32_t size;
		fread(&size, sizeof( uint32_t ), 1, fp);

		tile->offset = tile->size = size;
		tile->node = NULL;
		if ( tile->offset != 0 ) {
			tile->node = (int*)malloc(sizeof(int)*tile->offset);
			uint32_t j;
			for ( j = 0; j < size; j++ ) {
				uint32_t val;
				fread(&val, sizeof( uint32_t ), 1, fp);
				tile->node[j] = val;
			}
		}

		float x, z;
		fread(&x, sizeof( float ), 1, fp);
		fread(&z, sizeof( float ), 1, fp);

		tile->center.x = x;
		tile->center.y = 0;
		tile->center.z = z;

		int center_node;
		fread(&center_node, sizeof( int ), 1, fp);
		tile->center_node = center_node;
	}
	fclose(fp);

	mesh->tile_unit = tile_unit;
	mesh->tile_width = mesh->width / tile_unit + 1;
	mesh->tile_heigh = mesh->heigh / tile_unit + 1;

	
	//mesh->tile = create_tile(mesh, 400);

	struct vector3 result;

	count = 1024 * 10;

	double ti0 = get_time_millis();
	for ( int i = 1; i < count;i++ )
	{
		struct vector3 start = { 2833, 0, 34333 };
		struct vector3 over = { 49000, 0, 8333 };
		raycast(mesh, &start, &over, &result, NULL, NULL);
	}
	double ti1 = get_time_millis();

	printf("raycast:%f\n", ti1 - ti0);

	for ( int i = 1; i < count; i++ )
	{
		struct vector3 start = { 2833, 0, 34333 };
		struct vector3 over = { 49000, 0, 8333 };
		astar_find(mesh, &start, &over, NULL, NULL);
	}
	double ti2 = get_time_millis();

	printf("findpath:%f\n", ti2 - ti1);

	return 0;
}

#endif