def event_handler(e):
    code = e.get_code()
    obj = e.get_target_obj()
    if code == lv.EVENT.VALUE_CHANGED:
        option = " "*10
        obj.get_selected_str(option, len(option))
        print("Selected month: " + option.strip())

#
# An infinite roller with the name of the months
#

roller1 = lv.roller(lv.screen_active())
roller1.set_options("\n".join([
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"]),lv.roller.MODE.INFINITE)

roller1.set_visible_row_count(4)
roller1.center()
roller1.add_event_cb(event_handler, lv.EVENT.ALL, None)

