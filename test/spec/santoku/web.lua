<% template:push(os.getenv("TK_JPEG_WASM") == "1") %>

local assert = require("luassert")
local test = require("santoku.test")
local jpeg = require("santoku.jpeg")
local async = require("santoku.async")
local vec = require("santoku.vector")
local fs = require("santoku.fs")
local val = require("santoku.web.val")

collectgarbage("stop")

test("jpeg", function ()

  test("resize", function ()

    test("resizes a jpeg", function ()

      local ok, input_data = fs.readfile("res/image4.jpg")
      assert.equals(true, ok, input_data)

      local input_chunk_size = 100
      local input_len = #input_data
      local input_pos = 1

      local ok, scaler = jpeg.scale(1, 8, 35, 2500)
      assert(ok, scaler)

      local output_chunks = vec()

      return async.loop(function (loop, stop)

        if input_pos >= input_len then
          return stop(true)
        end

        return async.loop(function (loop, stop)

          local ok, status, output_chunk = scaler:read()

          assert.equals(true, ok, status)
          assert(status == jpeg.READ or status == jpeg.WRITE or status == jpeg.DONE)

          if status == jpeg.READ then
            output_chunks:append(output_chunk)
            return loop()
          else
            return stop(true, status)
          end

        end, function (ok, status)

          assert.equals(true, ok, status)

          if status == jpeg.DONE then
            return stop(true)
          end

          local next_pos = input_pos + input_chunk_size
          local input_chunk = val.bytes(input_data:sub(input_pos, next_pos))
          input_pos = next_pos + 1

          local ok, status = scaler:write(input_chunk:str())
          assert.equals(true, ok, status)

          return loop()

        end)

      end, function (ok, err)

        assert.equals(true, ok, err)
        local output_data = output_chunks:concat()
        local ok, err = fs.writefile("res/image4.smaller.jpg", output_data)
        assert.equals(true, ok, err)

      end)

    end)

  end)

end)

collectgarbage("collect")
val.global("gc"):call(nil)

val.global("setTimeout", function ()

  local cntt = 0
  for _ in pairs(val.IDX_REF_TBL) do
    cntt = cntt + 1
  end

  assert.equals(0, cntt, "IDX_REF_TBL not clean")

end, 5000)

<% template:pop() %>
