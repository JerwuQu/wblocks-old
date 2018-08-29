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

function block.mousedown()
    print("Mouse pressed!")
end

function block.mousescroll(delta)
    print("Scroll", delta)
end

a()
