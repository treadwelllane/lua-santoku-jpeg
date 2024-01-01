local test = require("santoku.test")
local jpeg = require("santoku.jpeg")
local vec = require("santoku.vector")
local fs = require("santoku.fs")

test("jpeg", function ()

  test("resize", function ()

    test("resizes a jpeg", function ()

      local ok, input_data = fs.readfile("test/res/image1.jpg")
      assert(ok == true, input_data)

      local input_chunk_size = 100
      local input_pos = 1

      local ok, scaler = jpeg.scale(1, 8, 35, 2500)
      assert(ok, scaler)

      local output_chunks = vec()

      while true do

        local ok, status, output_chunk = scaler:read()

        assert(ok == true, status)
        assert(status == jpeg.READ or status == jpeg.WRITE or status == jpeg.DONE, "unexpected status")

        if status == jpeg.READ then

          output_chunks:append(output_chunk)

        elseif status == jpeg.WRITE then

          local next_pos = input_pos + input_chunk_size
          local input_chunk = input_data:sub(input_pos, next_pos)
          input_pos = next_pos + 1

          local ok, status = scaler:write(input_chunk)
          assert(ok == true, status)

        else
          break
        end

      end

      local output_data = output_chunks:concat()
      local ok, status = fs.writefile("test/res/image1.smaller.jpg", output_data)
      assert(ok == true, status)

    end)

  end)

end)
