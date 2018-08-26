function a()
    SetText("a")
    SetColor(255, 0, 0)
    AddTimer(1000, b)
end

function b()
    SetText("b")
    SetColor(0, 255, 0)
    AddTimer(1000, a)
end

a()
