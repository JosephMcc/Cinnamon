#!/usr/bin/python3

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import GLib, Gio, Gtk, Gdk

class RunDialog(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self)

        self.set_keep_above(True)
        self.set_decorated(False)
        self.set_position(Gtk.WindowPosition.CENTER)
        self.set_resizable(False)
        self.set_border_width(12)
        self.set_default_size(240, -1)

        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        self.add(main_box)

        self.entry = Gtk.Entry()
        main_box.pack_start(self.entry, False, False, 0)

        button_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        main_box.pack_start(button_box, False, False, 0)

        button = Gtk.Button.new_with_label("Close")
        button.connect("clicked", self.on_button_clicked)
        button_box.pack_end(button, False, False, 0)

        self.show_all()

    def on_button_clicked(self, button):
        Gtk.main_quit()

if __name__ == "__main__":
    window = RunDialog()
    window.show_all()
    Gtk.main()
