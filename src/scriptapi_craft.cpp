/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "scriptapi.h"
#include "scriptapi_craft.h"

extern "C" {
#include <lauxlib.h>
}

#include "script.h"
#include "scriptapi_types.h"
#include "scriptapi_common.h"
#include "scriptapi_item.h"


struct EnumString es_CraftMethod[] =
{
	{CRAFT_METHOD_NORMAL, "normal"},
	{CRAFT_METHOD_COOKING, "cooking"},
	{CRAFT_METHOD_FUEL, "fuel"},
	{0, NULL},
};


// helper for register_craft
bool read_craft_recipe_shaped(lua_State *L, int index,
		int &width, std::vector<std::string> &recipe)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(!lua_istable(L, index))
		return false;

	lua_pushnil(L);
	int rowcount = 0;
	while(lua_next(L, index) != 0){
		int colcount = 0;
		// key at index -2 and value at index -1
		if(!lua_istable(L, -1))
			return false;
		int table2 = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table2) != 0){
			// key at index -2 and value at index -1
			if(!lua_isstring(L, -1))
				return false;
			recipe.push_back(lua_tostring(L, -1));
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			colcount++;
		}
		if(rowcount == 0){
			width = colcount;
		} else {
			if(colcount != width)
				return false;
		}
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
		rowcount++;
	}
	return width != 0;
}

// helper for register_craft
bool read_craft_recipe_shapeless(lua_State *L, int index,
		std::vector<std::string> &recipe)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(!lua_istable(L, index))
		return false;

	lua_pushnil(L);
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		if(!lua_isstring(L, -1))
			return false;
		recipe.push_back(lua_tostring(L, -1));
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	return true;
}

// helper for register_craft
bool read_craft_replacements(lua_State *L, int index,
		CraftReplacements &replacements)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(!lua_istable(L, index))
		return false;

	lua_pushnil(L);
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		if(!lua_istable(L, -1))
			return false;
		lua_rawgeti(L, -1, 1);
		if(!lua_isstring(L, -1))
			return false;
		std::string replace_from = lua_tostring(L, -1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 2);
		if(!lua_isstring(L, -1))
			return false;
		std::string replace_to = lua_tostring(L, -1);
		lua_pop(L, 1);
		replacements.pairs.push_back(
				std::make_pair(replace_from, replace_to));
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	return true;
}
// register_craft({output=item, recipe={{item00,item10},{item01,item11}})
int l_register_craft(lua_State *L)
{
	//infostream<<"register_craft"<<std::endl;
	luaL_checktype(L, 1, LUA_TTABLE);
	int table = 1;

	// Get the writable craft definition manager from the server
	IWritableCraftDefManager *craftdef =
			get_server(L)->getWritableCraftDefManager();

	std::string type = getstringfield_default(L, table, "type", "shaped");

	/*
		CraftDefinitionShaped
	*/
	if(type == "shaped"){
		std::string output = getstringfield_default(L, table, "output", "");
		if(output == "")
			throw LuaError(L, "Crafting definition is missing an output");

		int width = 0;
		std::vector<std::string> recipe;
		lua_getfield(L, table, "recipe");
		if(lua_isnil(L, -1))
			throw LuaError(L, "Crafting definition is missing a recipe"
					" (output=\"" + output + "\")");
		if(!read_craft_recipe_shaped(L, -1, width, recipe))
			throw LuaError(L, "Invalid crafting recipe"
					" (output=\"" + output + "\")");

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (output=\"" + output + "\")");
		}

		CraftDefinition *def = new CraftDefinitionShaped(
				output, width, recipe, replacements);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionShapeless
	*/
	else if(type == "shapeless"){
		std::string output = getstringfield_default(L, table, "output", "");
		if(output == "")
			throw LuaError(L, "Crafting definition (shapeless)"
					" is missing an output");

		std::vector<std::string> recipe;
		lua_getfield(L, table, "recipe");
		if(lua_isnil(L, -1))
			throw LuaError(L, "Crafting definition (shapeless)"
					" is missing a recipe"
					" (output=\"" + output + "\")");
		if(!read_craft_recipe_shapeless(L, -1, recipe))
			throw LuaError(L, "Invalid crafting recipe"
					" (output=\"" + output + "\")");

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (output=\"" + output + "\")");
		}

		CraftDefinition *def = new CraftDefinitionShapeless(
				output, recipe, replacements);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionToolRepair
	*/
	else if(type == "toolrepair"){
		float additional_wear = getfloatfield_default(L, table,
				"additional_wear", 0.0);

		CraftDefinition *def = new CraftDefinitionToolRepair(
				additional_wear);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionCooking
	*/
	else if(type == "cooking"){
		std::string output = getstringfield_default(L, table, "output", "");
		if(output == "")
			throw LuaError(L, "Crafting definition (cooking)"
					" is missing an output");

		std::string recipe = getstringfield_default(L, table, "recipe", "");
		if(recipe == "")
			throw LuaError(L, "Crafting definition (cooking)"
					" is missing a recipe"
					" (output=\"" + output + "\")");

		float cooktime = getfloatfield_default(L, table, "cooktime", 3.0);

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (cooking output=\"" + output + "\")");
		}

		CraftDefinition *def = new CraftDefinitionCooking(
				output, recipe, cooktime, replacements);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionFuel
	*/
	else if(type == "fuel"){
		std::string recipe = getstringfield_default(L, table, "recipe", "");
		if(recipe == "")
			throw LuaError(L, "Crafting definition (fuel)"
					" is missing a recipe");

		float burntime = getfloatfield_default(L, table, "burntime", 1.0);

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (fuel recipe=\"" + recipe + "\")");
		}

		CraftDefinition *def = new CraftDefinitionFuel(
				recipe, burntime, replacements);
		craftdef->registerCraft(def);
	}
	else
	{
		throw LuaError(L, "Unknown crafting definition type: \"" + type + "\"");
	}

	lua_pop(L, 1);
	return 0; /* number of results */
}

// get_craft_result(input)
int l_get_craft_result(lua_State *L)
{
	int input_i = 1;
	std::string method_s = getstringfield_default(L, input_i, "method", "normal");
	enum CraftMethod method = (CraftMethod)getenumfield(L, input_i, "method",
				es_CraftMethod, CRAFT_METHOD_NORMAL);
	int width = 1;
	lua_getfield(L, input_i, "width");
	if(lua_isnumber(L, -1))
		width = luaL_checkinteger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, input_i, "items");
	std::vector<ItemStack> items = read_items(L, -1);
	lua_pop(L, 1); // items

	IGameDef *gdef = get_server(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input(method, width, items);
	CraftOutput output;
	bool got = cdef->getCraftResult(input, output, true, gdef);
	lua_newtable(L); // output table
	if(got){
		ItemStack item;
		item.deSerialize(output.item, gdef->idef());
		LuaItemStack::create(L, item);
		lua_setfield(L, -2, "item");
		setintfield(L, -1, "time", output.time);
	} else {
		LuaItemStack::create(L, ItemStack());
		lua_setfield(L, -2, "item");
		setintfield(L, -1, "time", 0);
	}
	lua_newtable(L); // decremented input table
	lua_pushstring(L, method_s.c_str());
	lua_setfield(L, -2, "method");
	lua_pushinteger(L, width);
	lua_setfield(L, -2, "width");
	push_items(L, input.items);
	lua_setfield(L, -2, "items");
	return 2;
}

// get_craft_recipe(result item)
int l_get_craft_recipe(lua_State *L)
{
	int k = 0;
	char tmp[20];
	int input_i = 1;
	std::string o_item = luaL_checkstring(L,input_i);

	IGameDef *gdef = get_server(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input;
	CraftOutput output(o_item,0);
	bool got = cdef->getCraftRecipe(input, output, gdef);
	lua_newtable(L); // output table
	if(got){
		lua_newtable(L);
		for(std::vector<ItemStack>::const_iterator
			i = input.items.begin();
			i != input.items.end(); i++, k++)
		{
			if (i->empty())
			{
				continue;
			}
			sprintf(tmp,"%d",k);
			lua_pushstring(L,tmp);
			lua_pushstring(L,i->name.c_str());
			lua_settable(L, -3);
		}
		lua_setfield(L, -2, "items");
		setintfield(L, -1, "width", input.width);
		switch (input.method) {
		case CRAFT_METHOD_NORMAL:
			lua_pushstring(L,"normal");
			break;
		case CRAFT_METHOD_COOKING:
			lua_pushstring(L,"cooking");
			break;
		case CRAFT_METHOD_FUEL:
			lua_pushstring(L,"fuel");
			break;
		default:
			lua_pushstring(L,"unknown");
		}
		lua_setfield(L, -2, "type");
	} else {
		lua_pushnil(L);
		lua_setfield(L, -2, "items");
		setintfield(L, -1, "width", 0);
	}
	return 1;
}

// get_all_craft_recipes(result item)
int l_get_all_craft_recipes(lua_State *L)
{
	char tmp[20];
	int input_i = 1;
	std::string o_item = luaL_checkstring(L,input_i);
	IGameDef *gdef = get_server(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input;
	CraftOutput output(o_item,0);
	std::vector<CraftDefinition*> recipes_list = cdef->getCraftRecipes(output, gdef);
	if (recipes_list.empty())
	{
		lua_pushnil(L);
		return 1;
	}
	// Get the table insert function
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);
	lua_newtable(L);
	int table = lua_gettop(L);
	for(std::vector<CraftDefinition*>::const_iterator
		i = recipes_list.begin();
		i != recipes_list.end(); i++)
	{
		CraftOutput tmpout;
		tmpout.item = "";
		tmpout.time = 0;
		CraftDefinition *def = *i;
		tmpout = def->getOutput(input, gdef);
		if(tmpout.item.substr(0,output.item.length()) == output.item)
		{
			input = def->getInput(output, gdef);
			lua_pushvalue(L, table_insert);
			lua_pushvalue(L, table);
			lua_newtable(L);
			int k = 0;
			lua_newtable(L);
			for(std::vector<ItemStack>::const_iterator
				i = input.items.begin();
				i != input.items.end(); i++, k++)
			{
				if (i->empty()) continue;
				sprintf(tmp,"%d",k);
				lua_pushstring(L,tmp);
				lua_pushstring(L,i->name.c_str());
				lua_settable(L, -3);
			}
			lua_setfield(L, -2, "items");
			setintfield(L, -1, "width", input.width);
			switch (input.method)
				{
				case CRAFT_METHOD_NORMAL:
					lua_pushstring(L,"normal");
					break;
				case CRAFT_METHOD_COOKING:
					lua_pushstring(L,"cooking");
					break;
				case CRAFT_METHOD_FUEL:
					lua_pushstring(L,"fuel");
					break;
				default:
					lua_pushstring(L,"unknown");
				}
			lua_setfield(L, -2, "type");
			if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
		}
	}
	return 1;
}
