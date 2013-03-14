/*
Minetest-c55
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

#ifndef LUA_ITEM_H_
#define LUA_ITEM_H_

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <vector>

#include "itemdef.h"
#include "content_sao.h"
#include "util/pointedthing.h"
#include "inventory.h"

#include "inventory.h"

ItemStack read_item(lua_State *L, int index);
std::vector<ItemStack> read_items(lua_State *L, int index);
void push_items(lua_State *L, const std::vector<ItemStack> &items);

class LuaItemStack
{
private:
	ItemStack m_stack;

	static const char className[];
	static const luaL_reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// is_empty(self) -> true/false
	static int l_is_empty(lua_State *L);

	// get_name(self) -> string
	static int l_get_name(lua_State *L);

	// get_count(self) -> number
	static int l_get_count(lua_State *L);

	// get_wear(self) -> number
	static int l_get_wear(lua_State *L);

	// get_metadata(self) -> string
	static int l_get_metadata(lua_State *L);

	// clear(self) -> true
	static int l_clear(lua_State *L);

	// replace(self, itemstack or itemstring or table or nil) -> true
	static int l_replace(lua_State *L);

	// to_string(self) -> string
	static int l_to_string(lua_State *L);

	// to_table(self) -> table or nil
	static int l_to_table(lua_State *L);

	// get_stack_max(self) -> number
	static int l_get_stack_max(lua_State *L);

	// get_free_space(self) -> number
	static int l_get_free_space(lua_State *L);

	// is_known(self) -> true/false
	// Checks if the item is defined.
	static int l_is_known(lua_State *L);

	// get_definition(self) -> table
	// Returns the item definition table from minetest.registered_items,
	// or a fallback one (name="unknown")
	static int l_get_definition(lua_State *L);

	// get_tool_capabilities(self) -> table
	// Returns the effective tool digging properties.
	// Returns those of the hand ("") if this item has none associated.
	static int l_get_tool_capabilities(lua_State *L);

	// add_wear(self, amount) -> true/false
	// The range for "amount" is [0,65535]. Wear is only added if the item
	// is a tool. Adding wear might destroy the item.
	// Returns true if the item is (or was) a tool.
	static int l_add_wear(lua_State *L);

	// add_item(self, itemstack or itemstring or table or nil) -> itemstack
	// Returns leftover item stack
	static int l_add_item(lua_State *L);

	// item_fits(self, itemstack or itemstring or table or nil) -> true/false, itemstack
	// First return value is true iff the new item fits fully into the stack
	// Second return value is the would-be-left-over item stack
	static int l_item_fits(lua_State *L);

	// take_item(self, takecount=1) -> itemstack
	static int l_take_item(lua_State *L);

	// peek_item(self, peekcount=1) -> itemstack
	static int l_peek_item(lua_State *L);

public:
	LuaItemStack(const ItemStack &item);
	~LuaItemStack();

	const ItemStack& getItem() const;
	ItemStack& getItem();

	// LuaItemStack(itemstack or itemstring or table or nil)
	// Creates an LuaItemStack and leaves it on top of stack
	static int create_object(lua_State *L);
	// Not callable from Lua
	static int create(lua_State *L, const ItemStack &item);
	static LuaItemStack* checkobject(lua_State *L, int narg);
	static void Register(lua_State *L);

};

/*****************************************************************************/
/* Mod API                                                                   */
/*****************************************************************************/
int l_register_item_raw(lua_State *L);
int l_register_alias_raw(lua_State *L);

/*****************************************************************************/
/* scriptapi internal                                                        */
/*****************************************************************************/
bool get_item_callback(lua_State *L,
		const char *name, const char *callbackname);
ItemDefinition read_item_definition(lua_State *L, int index,
		ItemDefinition default_def = ItemDefinition());

extern struct EnumString es_ItemType[];

/*****************************************************************************/
/* Minetest interface                                                        */
/*****************************************************************************/
bool scriptapi_item_on_drop(lua_State *L, ItemStack &item,
		ServerActiveObject *dropper, v3f pos);
bool scriptapi_item_on_place(lua_State *L, ItemStack &item,
		ServerActiveObject *placer, const PointedThing &pointed);
bool scriptapi_item_on_use(lua_State *L, ItemStack &item,
		ServerActiveObject *user, const PointedThing &pointed);


#endif /* LUA_ITEM_H_ */
