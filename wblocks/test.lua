local ffi = require("ffi")
ffi.cdef[[
void Sleep(int ms);
]]

while true do
    SetText("a")
    ffi.C.Sleep(1000)
    SetText("b")
    ffi.C.Sleep(1000)
    SetText("c")
    ffi.C.Sleep(1000)
end
