
# programmatic UI

type MainWindow extends Window
    var lb = OutlineView(pos=0x0, size=500x400)
end type

var wMain = MainWindow()
show(window=wMain, pos=100x100, size=800x600)


#declarative

MainWindow:
    name = "wMain"
    OutlineView:
        # actually this is just a constructor or initfn being defined
        name = "lb"
        pos = 0x0
        size = mkSize(width=sin(800), height=600)
    HStack:
        VStack:
            Button:
                text = "Now!"
                onClick = ...
        Label:
            width = 400
            text = "The quick brown fox"
