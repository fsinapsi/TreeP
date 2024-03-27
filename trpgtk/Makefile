
myname=	 gtk
mylibs=	 ../libs/libtrp$(myname).a ../libs/libtrp$(myname).so
myobjs=	 trp$(myname).o trpgtk_internal.o trpgtk_callbacks.o
myobjs+= trpgtk_widget.o trpgtk_container.o trpgtk_dialog.o
myobjs+= trpgtk_window.o
myobjs+= trpgtk_adjustment.o trpgtk_scrolled_window.o
myobjs+= trpgtk_box.o trpgtk_label.o trpgtk_button.o trpgtk_toggle_button.o trpgtk_check_button.o
myobjs+= trpgtk_file_chooser.o trpgtk_file_chooser_dialog.o trpgtk_file_chooser_widget.o
myobjs+= trpgtk_color_button.o trpgtk_color_selection.o trpgtk_color_selection_dialog.o
myobjs+= trpgtk_event_box.o trpgtk_editable.o trpgtk_entry.o trpgtk_spin_button.o
myobjs+= trpgtk_scale.o trpgtk_range.o trpgtk_combo_box.o trpgtk_combo_box_text.o
myobjs+= trpgtk_text_view.o trpgtk_text_buffer.o
myobjs+= trpgtk_notebook.o trpgtk_image.o trpgtk_progress_bar.o
myobjs+= trpgtk_font_button.o trpgtk_frame.o trpgtk_separator.o
myobjs+= trpgtk_menu_item.o trpgtk_menu_bar.o trpgtk_menu.o trpgtk_menu_shell.o
myobjs+= trpgtk_tree_view.o
myobjs+= trpgtk_message_dialog.o
myobjs+= trpgtk_calendar.o
myobjs+= trpgtk_drag.o
myobjs+= trpgtk_image_graph.o

CFLAGS= `cat ../.cflags`
ifeq ($(TARGET), i686-w64-mingw32)
CFLAGS+= `i686-w64-mingw32-pkg-config --cflags gtk+-2.0`
else
ifeq ($(TARGET), x86_64-w64-mingw32)
CFLAGS+= `x86_64-w64-mingw32-pkg-config --cflags gtk+-2.0`
else
CFLAGS+= `pkg-config --cflags gtk+-2.0`
endif
endif
LDFLAGS= `cat ../.ldflags`

all:			$(mylibs)

clean:
	rm -f *~ $(mylibs) $(myobjs)

$(mylibs):		$(myobjs)
	rm -f $(mylibs)
	$(AR) rcs ../libs/libtrp$(myname).a $(myobjs)
ifeq ($(TARGET), Linux)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libtrp$(myname).so -o ../libs/libtrp$(myname).so $(myobjs)
endif

$(myobjs):		../trp/trp.h trpgtk.h trpgtk_internal.h

$%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

trpgtk_widget.o trpgtk_text_buffer.o trpgtk_image.o trpgtk_color_button.o trpgtk_color_selection.o:	../trppix/trppix.h ../trppix/trppix_internal.h
trpgtk_image_graph.o:	../trppix/trppix.h ../trppix/trppix_internal.h ../trpvid/trpvid_internal.h

