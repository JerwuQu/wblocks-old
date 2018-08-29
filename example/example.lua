function printTable(t)
    for k, v in pairs(t) do
        print(k, v)
    end
end

function a()
    block.setText("a")
    block.setColor(255, 0, 0)
    timer.add(1000, b)
end

function b()
    block.setText("b")
    block.setColor(0, 255, 0)
    timer.add(1000, a)
end

function block.mousedown(modifiers)
    print("Mouse pressed!")
    printTable(modifiers)
end

function block.mousescroll(delta, modifiers)
    print("Scroll", delta)
    printTable(modifiers)
end

a()
