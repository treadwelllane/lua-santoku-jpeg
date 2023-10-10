local assert = require("luassert")
local test = require("santoku.test")
local jpeg = require("santoku.jpeg")
local gen = require("santoku.gen")
local fun = require("santoku.fun")
local vec = require("santoku.vector")
local tup = require("santoku.tuple")
local fs = require("santoku.fs")

test("jpeg", function ()

  test("resize", function ()

    test("resizes a jpeg (no coroutines)", function ()

      local ok, input_data = fs.readfile("test/spec/santoku/jpeg/image.jpg")
      assert.equals(true, ok)

      local input_chunk_size = 100
      local input_len = #input_data
      local input_pos = 1
      local output_chunks = vec()

      jpeg.scale(
        function ()
          if input_pos < input_len then
            local next_pos = input_pos + input_chunk_size
            local chunk = input_data:sub(input_pos, next_pos)
            input_pos = next_pos + 1
            return chunk
          end
        end,
        function (chunk)
          output_chunks:append(chunk)
        end,
        1, 8, 35, 2500)

      local output_data = output_chunks:concat()
      local ok = fs.writefile("test/spec/santoku/jpeg/image.smaller.noco.jpg", output_data)
      assert.equals(true, ok)

    end)

    test("resizes a jpeg (coroutines)", function ()

      local ok, input_data = fs.readfile("test/spec/santoku/jpeg/image.jpg")
      assert.equals(true, ok)

      local input_chunks = gen(function (yield)
        local input_chunk_size = 100
        local input_len = #input_data
        local input_pos = 1
        while input_pos < input_len do
          local next_pos = input_pos + input_chunk_size
          local chunk = input_data:sub(input_pos, next_pos)
          input_pos = next_pos + 1
          yield(chunk)
        end
      end):co()

      local output_chunks = gen(function (yield)
        return jpeg.scale(
          input_chunks,
          yield,
          1, 8, 35, 2500)
      end):co()

      local output_data = output_chunks:concat()
      local ok = fs.writefile("test/spec/santoku/jpeg/image.smaller.co.jpg", output_data)
      assert.equals(true, ok)

    end)

  end)

end)
