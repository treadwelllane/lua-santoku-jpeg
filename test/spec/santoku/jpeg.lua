local assert = require("luassert")
local test = require("santoku.test")
local jpeg = require("santoku.jpeg")
local err = require("santoku.err")
local vec = require("santoku.vector")
local fs = require("santoku.fs")

test("jpeg", function ()

  test("resize", function ()

    -- TODO: Ideally it should look like this:
    --
    -- test("resizes a jpeg", function ()
    --   assert(err.pwrap(function (check)
    --     -- Generator producing file chunks
    --     local original_chunks = check(fs.chunks("test/spec/santoku/jpeg/image.jpg", 100))
    --     -- Generator producing scaled chunks
    --     local scaled_chunks = check(jpeg.scaler(original_chunks, 8, 2000))
    --     -- Run generator and concat produced chunks
    --     local data = scaled_chunks:map(check):concat()
    --     -- Write the file out
    --     check(fs.writefile("test/spec/santoku/jpeg/image.smaller.jpg", data))
    --   end))
    -- end)

    test("resizes a jpeg", function ()

      local ok, input = fs.readfile("test/spec/santoku/jpeg/image.jpg")
      assert.equals(true, ok)

      local chunk_size = 100
      local input_len = #input
      local input_pos = 1

      local output = vec()

      local ok, err = jpeg.scale(function ()
        local next_pos = input_pos + chunk_size
        local out = input:sub(input_pos, next_pos)
        input_pos = next_pos + 1
        print("pull data", input_pos, #out)
        return out, #out
      end, function (data, len)
        print("push_data", len)
        output:append(data)
      end, 1, 8, 35, 2500)

      print(ok, err)
      assert(ok, err)

      output = output:concat()

      local ok = fs.writefile("test/spec/santoku/jpeg/image.smaller.jpg", output)

      assert.equals(true, ok)

    end)

  end)

end)
