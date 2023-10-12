local assert = require("luassert")
local test = require("santoku.test")
local jpeg = require("santoku.jpeg")
local gen = require("santoku.gen")
local fun = require("santoku.fun")
local async = require("santoku.async")
local vec = require("santoku.vector")
local tup = require("santoku.tuple")
local fs = require("santoku.fs")

test("jpeg", function ()

  test("resize", function ()

    test("resizes a jpeg", function ()

      local ok, input_data = fs.readfile("test/spec/santoku/jpeg/image.jpg")
      assert.equals(true, ok)

      local input_chunk_size = 100
      local input_len = #input_data
      local input_pos = 1

      local ok, scaler = jpeg.scale(1, 8, 35, 2500)
      assert(ok, scaler)

      local output_chunks = vec()

      while input_pos < input_len do

        local next_pos = input_pos + input_chunk_size
        local input_chunk = input_data:sub(input_pos, next_pos)
        input_pos = next_pos + 1

        local ok, status = scaler:write(input_chunk)
        assert.equals(true, ok, status)

        while true do

          local ok, status, output_chunk = scaler:read()

          assert.equals(true, ok)

          assert(status == jpeg.READ or status == jpeg.WRITE)

          if status == jpeg.READ then
            output_chunks:append(output_chunk)
          else
            break
          end

        end

      end

      local output_data = output_chunks:concat()
      local ok = fs.writefile("test/spec/santoku/jpeg/image.smaller.jpg", output_data)
      assert.equals(true, ok)

    end)

  end)

end)
