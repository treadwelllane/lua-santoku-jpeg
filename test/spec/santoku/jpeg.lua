local test = require("santoku.test")
local arr = require("santoku.array")
local jpeg = require("santoku.jpeg")
local fs = require("santoku.fs")

test("jpeg", function ()

  test("resize", function ()

    test("resizes a jpeg", function ()

      local input_data = fs.readfile("test/res/image1.jpg")

      local input_chunk_size = 100
      local input_pos = 1

      local scaler = jpeg.scale(1, 8, 35, 2500)

      local output_chunks = {}

      while true do

        local status, output_chunk = scaler:read()

        assert(status == jpeg.READ or status == jpeg.WRITE or status == jpeg.DONE, "unexpected status")

        if status == jpeg.READ then

          arr.push(output_chunks, output_chunk)

        elseif status == jpeg.WRITE then

          local next_pos = input_pos + input_chunk_size
          local input_chunk = input_data:sub(input_pos, next_pos)
          input_pos = next_pos + 1

          scaler:write(input_chunk)

        else
          break
        end

      end

      local output_data = arr.concat(output_chunks)
      fs.writefile("test/res/image1.smaller.jpg", output_data)

    end)

  end)

end)
