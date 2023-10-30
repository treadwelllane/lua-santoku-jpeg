<% vec = require("santoku.vector") %>
<% str = require("santoku.string") %>

package = "<% return name %>"
version = "<% return version %>"
rockspec_format = "3.0"

source = {
  url = "<% return download %>",
}

description = {
  homepage = "<% return homepage %>",
  license = "<% return license %>"
}

dependencies = {
  <% return vec.wrap(dependencies):map(str.quote):concat(",\n") %>
}

test_dependencies = {
  <% return vec.wrap(test_dependencies):map(str.quote):concat(",\n") %>
}

build = {
  type = "make",
  variables = {
    LIB_EXTENSION = "$(LIB_EXTENSION)",
  },
  build_variables = {
    CC = "$(CC)",
    CFLAGS = "$(CFLAGS)",
    LIBFLAG = "$(LIBFLAG)",
    LUA_BINDIR = "$(LUA_BINDIR)",
    LUA_INCDIR = "$(LUA_INCDIR)",
    LUA_LIBDIR = "$(LUA_LIBDIR)",
    LUA = "$(LUA)",
  },
  install_variables = {
    INST_PREFIX = "$(PREFIX)",
    INST_BINDIR = "$(BINDIR)",
    INST_LIBDIR = "$(LIBDIR)",
    INST_LUADIR = "$(LUADIR)",
    INST_CONFDIR = "$(CONFDIR)",
  }
}

test = {
  type = "command",
  command = "make test"
}
