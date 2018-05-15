function a()
    SetText("a")
    AddTimer(1000, b)
end

function b()
    SetText("b")
    AddTimer(1000, a)
end

a()
