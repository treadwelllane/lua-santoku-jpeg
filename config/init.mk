NAME = <% return name %>
VERSION = <% return version %>
VPFX = <% return variable_prefix %>

export LUA = <% return os.getenv("LUA") %>
export LUA_PATH = <% return os.getenv("LUA_PATH") %>
export LUA_CPATH = <% return os.getenv("LUA_CPATH") %>
