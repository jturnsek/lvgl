#
# A simple grid
#

col_dsc = [70, 70, 70, lv.GRID_TEMPLATE_LAST]
row_dsc = [50, 50, 50, lv.GRID_TEMPLATE_LAST]

# Create a container with grid
cont = lv.obj(lv.screen_active())
cont.set_style_grid_column_dsc_array(col_dsc, 0)
cont.set_style_grid_row_dsc_array(row_dsc, 0)
cont.set_size(300, 220)
cont.center()
cont.set_layout(lv.LAYOUT.GRID)

for i in range(9):
    col = i % 3
    row = i // 3

    obj = lv.button(cont)
    # Stretch the cell horizontally and vertically too
    # Set span to 1 to make the cell 1 column/row sized
    obj.set_grid_cell(lv.GRID_ALIGN.STRETCH, col, 1,
                      lv.GRID_ALIGN.STRETCH, row, 1)

    label = lv.label(obj)
    label.set_text("c" +str(col) +  "r" +str(row))
    label.center()

