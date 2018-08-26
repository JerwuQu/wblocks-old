function a()
    block.setText("a")
    block.setColor(255, 0, 0)
    block.addTimer(1000, b)
end

function b()
    block.setText("b")
    block.setColor(0, 255, 0)
    block.addTimer(1000, a)
end

function block.mousedown()
    print("Mouse pressed!")
end

a()
