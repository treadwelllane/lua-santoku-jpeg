-- TODO: Revise async tests in light of santoku upgrade (no more async.loop,
-- etc. Do we need it?)

-- luacheck: ignore
--
-- local test = require("santoku.test")
-- local arr = require("santoku.array")
-- local jpeg = require("santoku.jpeg")
-- local fs = require("santoku.fs")
--
-- test("jpeg", function ()
--
--   test("resize", function ()
--
--     test("resizes a jpeg", function ()
--
--       local input_data = fs.readfile("test/res/image4.jpg")
--
--       local input_chunk_size = 100
--       local input_len = #input_data
--       local input_pos = 1
--
--       local scaler = jpeg.scale(1, 8, 35, 2500)
--
--       local output_chunks = {}
--
--       return async.loop(function (loop, stop)
--
--         if input_pos >= input_len then
--           return stop(true)
--         end
--
--         return async.loop(function (loop, stop)
--
--           local ok, status, output_chunk = scaler:read()
--
--           assert(ok == true, status)
--           assert(status == jpeg.READ or status == jpeg.WRITE or status == jpeg.DONE, "unexpected status")
--
--           if status == jpeg.READ then
--             arr.push(output_chunks, output_chunk)
--             return loop()
--           else
--             return stop(true, status)
--           end
--
--         end, function (ok, status)
--
--           assert(ok == true, status)
--
--           if status == jpeg.DONE then
--             return stop(true)
--           end
--
--           local next_pos = input_pos + input_chunk_size
--           local input_chunk_str = input_data:sub(input_pos, next_pos)
--           input_pos = next_pos + 1
--
--           local ok, status = scaler:write(input_chunk_str)
--           assert(ok == true, status)
--
--
--           return loop()
--
--         end)
--
--       end, function (ok, err)
--
--         assert(ok == true, err)
--         local output_data = arr.concat(output_chunks)
--         fs.writefile("test/res/image4.smaller.jpg", output_data)
--
--       end)
--
--     end)
--
--   end)
--
-- end)
