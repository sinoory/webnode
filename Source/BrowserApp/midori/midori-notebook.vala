/*
 Copyright (C) 2013 Christian Dywan <christian@twotoasts.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/
  //The .vala file is modified by wangyl to dynamically change the position of the button which is used for adding a new bookmark In 2015.5.18

namespace Midori {
    protected class Tally : Gtk.EventBox {
        public Midori.Tab tab { get; set; }
//add by luyue 2015/2/13 start
        Gtk.Widget spinner;
        Gdk.Pixbuf pixbuf;
        Gdk.PixbufSimpleAnim animation;
        
//        Gtk.Spinner spinner;
//add end
        public Gtk.Label label;
        Gtk.HBox box;
        public Gtk.Image icon;
        public Gtk.Image loc_simbo_icon;
        Gtk.Alignment align;

        public Gtk.Button ? block_simbo { get; set; default = null; }
        public string ? block_uri_title { get; set; default = null; }
        public Gtk.Image block_simbo_icon;

        Gtk.HBox hbox;
        public Gtk.Button ? loc_simbo { get; set; default = null; }

        Gtk.Button close;

        public bool close_button_left { get; set; default = false; }
        public bool close_button_visible { get; set; default = false; }

        public bool loc_simbo_button_left { get; set; default = false; }			//lxx, 20150129
        public bool loc_simbo_button_visible { get; set; default = false; }		//lxx, 20150129

        public bool track_location { get; set; default = false; }		//lxx, 20150205

        protected Tally (Midori.Tab tab) {
            this.tab = tab;
            box = new Gtk.HBox (false, 1);
            add (box);

//add by luyue 2015/2/13 start 
//            spinner = new Gtk.Spinner ();
//            spinner.active = true;
           animation = new Gdk.PixbufSimpleAnim(16,16,5);
           pixbuf = new Gdk.Pixbuf.from_file(Midori.Paths.get_res_filename("spinner1.png"));
           animation.add_frame(pixbuf);
           pixbuf = new Gdk.Pixbuf.from_file(Midori.Paths.get_res_filename("spinner2.png"));
           animation.add_frame(pixbuf);
           pixbuf = new Gdk.Pixbuf.from_file(Midori.Paths.get_res_filename("spinner3.png"));
           animation.add_frame(pixbuf);
           pixbuf = new Gdk.Pixbuf.from_file(Midori.Paths.get_res_filename("spinner4.png"));
           animation.add_frame(pixbuf);
           animation.set_loop(true);
           spinner = new Gtk.Image.from_animation(animation);
        //    spinner.active = true;
            /* Ensure the spinner is the size of the icon */
       //     int icon_size = 16;
       //     Gtk.icon_size_lookup_for_settings (get_settings (),
       //        Gtk.IconSize.MENU, out icon_size, null);
       //     spinner.set_size_request (icon_size, icon_size);
//add end
            box.pack_start (spinner, false, false, 0);
            label = new Gtk.Label (null);
            label.set_alignment (0.5f, 0.5f);
            label.set_padding (2, 2);
            box.pack_start (label, true, true, 0);
            close = new Gtk.Button ();
            close.relief = Gtk.ReliefStyle.NONE;
            close.focus_on_click = false;

            loc_simbo = new Gtk.Button ();															//lxx, 20150129
            loc_simbo.relief = Gtk.ReliefStyle.NONE;												//lxx, 20150129
            loc_simbo.focus_on_click = false;														//lxx, 20150129

            block_simbo = new Gtk.Button ();															//lxx, 20150129
            block_simbo.relief = Gtk.ReliefStyle.NONE;												//lxx, 20150129
            block_simbo.focus_on_click = false;														//lxx, 20150129

#if !HAVE_GTK3
            close.name = "midori-close-button";
            close.style_set.connect (close_style_set);

            loc_simbo.name = "midori-loc_simbo-button";														//lxx, 20150129
            loc_simbo.style_set.connect (midori-loc_style_set);											//lxx, 20150129

            block_simbo.name = "midori-block-simbo-button";														//lxx, 20150129
            block_simbo.style_set.connect (midori-block_style_set);											//lxx, 20150129
#endif
            icon = new Gtk.Image.from_gicon (new ThemedIcon.with_default_fallbacks ("window-close-symbolic"), Gtk.IconSize.MENU);
            close.add (icon);
            align = new Gtk.Alignment (1.0f, 0.5f, 0.0f, 0.0f);
				hbox = new Gtk.HBox(false, 0);
            align.add (hbox);
//lxx, 20150129
            hbox.add (block_simbo);
            hbox.add (loc_simbo);
            hbox.add (close);
//lxx, 20150129
            box.pack_start (align, false, false, 0);
            close.clicked.connect (close_clicked);
				loc_simbo.clicked.connect(loc_simbo_clicked);
				block_simbo.clicked.connect(block_simbo_clicked);
            icon = new Gtk.Image.from_gicon (new ThemedIcon.with_default_fallbacks ("text-html-symbolic"), Gtk.IconSize.MENU);
            box.pack_start (icon, false, false, 0);
            box.show_all ();
	    loc_simbo.hide();//lxx,20150130
            block_simbo.hide();//lxxx,2015/3/27
            tab.notify["uri"].connect (uri_changed);
            tab.notify["title"].connect (title_changed);
            tab.notify["icon"].connect (icon_changed);
            tab.notify["minimized"].connect (minimized_changed);
            tab.notify["progress"].connect (progress_changed);
            tab.colors_changed.connect (colors_changed);
            update_label ();
            label.visible = !tab.minimized;
            spinner.visible = tab.progress > 0.0;
            icon.visible = !spinner.visible;
            update_color ();

            notify["close-button-left"].connect (close_button_left_changed);
            notify_property ("close-button-left");
            notify["close-button-visible"].connect (close_button_visible_changed);
            notify_property ("close-button-visible");
        }

#if !HAVE_GTK3
        void close_style_set (Gtk.Style? previous_style) {
            Gtk.Requisition size;
            close.child.size_request (out size);
            close.set_size_request (size.width, size.height);
        }
#endif

        void close_clicked () {
            tab.destroy ();
        }
//lxx, 20150215
	void block_simbo_clicked()
	{
		string output;
		if(0 == block_uri_title.length)
		{
			output = string.join ("\n", "已拦截此网页下的下列弹出式窗口：", "about:blank","如需允许，请到首选项中的内容界面进行设置。");
		}
		else
		{
			output = string.join ("\n", "已拦截此网页下的下列弹出式窗口：", block_uri_title,"如需允许，请到首选项中的内容界面进行设置。");
		}

		Gtk.Dialog dialog = new Gtk.MessageDialog(null,
												 Gtk.DialogFlags.DESTROY_WITH_PARENT,
												 Gtk.MessageType.INFO/*WARNING*/, Gtk.ButtonsType.NONE, output);
		
//		stdout.printf("this is block printing %s	%d\n", block_uri_title, block_uri_title.length);

		dialog.title = "提示";
		dialog.add_buttons (/*"管理位置设置", Gtk.ResponseType.CANCEL,*/ "完成",  Gtk.ResponseType.ACCEPT);
		bool cancel = dialog.run () != Gtk.ResponseType.ACCEPT;
		dialog.destroy ();
		if(cancel)
		{
//			stdout.printf("this is vala printing\n");
		}
	}
//lxx, 20150205
	void loc_simbo_clicked () 
	{
		string uriHost = tab.uri;
		Gtk.Dialog dialog;
		int index = uriHost.index_of_char('/');
		string ihost = uriHost[index+2:uriHost.length];
		index = ihost.index_of_char('/');
		string host = ihost[0:index];

		if(track_location)
		{
			string output = string.join ("\n", "此网页包含来自以下网站的元素，这些网站正在跟踪您的位置：", host,"如需阻止，请到首选项中的隐私界面进行设置。");
			dialog = new Gtk.MessageDialog(null,
													 Gtk.DialogFlags.DESTROY_WITH_PARENT,
													 Gtk.MessageType.INFO/*WARNING*/, Gtk.ButtonsType.NONE, output);
		}
		else
		{
			string output = string.join ("\n", "系统已阻止以下网站跟踪您在此网页上的位置：", host,"如需允许，请到首选项中的隐私界面进行设置。");
			dialog = new Gtk.MessageDialog(null,
													 Gtk.DialogFlags.DESTROY_WITH_PARENT,
													 Gtk.MessageType.WARNING, Gtk.ButtonsType.NONE, output);
		}

		dialog.title = "提示";
		dialog.add_buttons (/*"管理位置设置", Gtk.ResponseType.CANCEL,*/ "完成",  Gtk.ResponseType.ACCEPT);
		bool cancel = dialog.run () != Gtk.ResponseType.ACCEPT;
		dialog.destroy ();
                
	}

        void uri_changed (GLib.ParamSpec pspec) {
            label.label = tab.uri;
        }

        void title_changed (GLib.ParamSpec pspec) {
            update_label ();
        }

        void update_label () {
            string? title;
            tab.get ("title", out title);
            label.label = Midori.Tab.get_display_title (title, tab.uri);
            /* Use computed label below! */
            label.ellipsize = Midori.Tab.get_display_ellipsize (label.label, tab.uri);
            tooltip_text = label.label;
        }

        void icon_changed (GLib.ParamSpec pspec) {
            Icon? icon;
            tab.get ("icon", out icon);
            this.icon.set_from_gicon (icon, Gtk.IconSize.MENU);
        }

        void colors_changed () {
            update_color ();
        }

        void update_color () {
            visible_window = tab.fg_color != null || tab.bg_color != null;
            label.modify_fg (Gtk.StateType.NORMAL, tab.fg_color);
            label.modify_fg (Gtk.StateType.ACTIVE, tab.fg_color);
            modify_bg (Gtk.StateType.NORMAL, tab.bg_color);
            modify_bg (Gtk.StateType.ACTIVE, tab.bg_color);
        }

        void close_button_left_changed (GLib.ParamSpec pspec) {
            if (close_button_left) {
                box.reorder_child (align, 0);
                box.reorder_child (label, 1);
                box.reorder_child (icon, 2);
                box.reorder_child (spinner, 3);
            } else {
                box.reorder_child (spinner, 0);
                box.reorder_child (icon, 1);
                box.reorder_child (label, 2);
                box.reorder_child (align, 3);
            }
        }

        void close_button_visible_changed (GLib.ParamSpec pspec) {
            align.visible = !tab.minimized && close_button_visible;
        }

//lxx, 20150130
        void loc_simbo_button_visible_changed (GLib.ParamSpec pspec) { 
            align.visible = !tab.minimized && loc_simbo_button_visible;
        }
//lxx, 20150130

        void minimized_changed (GLib.ParamSpec pspec) {
            label.visible = !tab.minimized;
            notify_property ("close-button-visible");
        }

        void progress_changed (GLib.ParamSpec pspec) {
            spinner.visible = tab.progress > 0.0;
            icon.visible = !spinner.visible;
       //       icon.visible = !(tab.progress > 0.0);
        }
    }
//Notebook class init here, lxx, 20150201
    public class Notebook : Gtk.EventBox {
        public Gtk.Notebook  notebook;
        int last_tab_size = 0;
        
      //  bool notebook_tab_press = false;
        public Gtk.Button add_tab_btn;//the button to add a new bookmark
        public int have_arrow = 1;//-1 represents no arrows, 0 represents have arrows, 1 represents unknowned
        public bool  btn_end  = false;//false represents that button  is placed at the end,ture  first 
#if !HAVE_GTK3
        static const string style_fixup = """
            style "midori-close-button-style"
            {
            GtkWidget::focus-padding = 0
            GtkWidget::focus-line-width = 0
            xthickness = 0
            ythickness = 0
            }
            widget "*.midori-close-button" style "midori-close-button-style"
            """;
#endif

//        public Tally ? tally { get; set; default = null; }//lxx, 20150130
        /* Since: 0.5.7 */
        public uint count { get; private set; default = 0; }
        /* Since: 0.5.7 */
        public int index { get; set; default = -1; }
        /* Since: 0.5.7 */
        public Midori.Tab? tab {  get; set; default = null; }
        /* Since: 0.5.7 */
        private Midori.Tab? previous {  get; set; default = null; }

        /* Since: 0.5.7 */
        public bool close_buttons_left { get; set; default = true; }
        /* Since: 0.5.7 */
        public bool close_buttons_visible { get; set; default = true; }

        public bool loc_simbo_buttons_left { get; set; default = false; }//lxx, 20150129
        /* Since: 0.5.7 */
        public bool loc_simbo_buttons_visible { get; set; default = false; }//lxx, 20150129

        /* Since: 0.5.7 */
        public bool labels_visible { get; set; default = true; }

        /* Since: 0.5.7 */
        public signal void tab_context_menu (Midori.Tab tab, ContextAction menu);
        /* Since: 0.5.7 */
        public signal void context_menu (ContextAction menu);
        /* The current tab is about to switch, but the old tab still has focus.
           Since: 0.5.7 */
        public signal void tab_switched (Midori.Tab? old, Midori.Tab @new);
        /* A tab is about to move to a new position.
           Since: 0.5.7 */
        public signal void tab_moved (Midori.Tab tab, uint new_index);
        /* A tab is being dragging out of the window.
           Since: 0.5.7 */
        public signal void tab_detached (Midori.Tab tab, int x, int y);
        /* Since: 0.5.7 */
        public signal void new_tab ();

        [CCode (type = "GtkWidget*")]
        public Notebook () {
            visible_window = false;
            notebook = new Gtk.Notebook ();
            notebook.visible =  notebook.scrollable = true;
            notebook.show_border = false;
            notebook.set ("group-name", PACKAGE_NAME);
            add (notebook);
#if HAVE_GTK3
            get_style_context ().add_class ("dynamic-notebook");
#else
            /* Remove the inner border between scrollbars and window border */
            Gtk.RcStyle rcstyle = new Gtk.RcStyle ();
            rcstyle.xthickness = 0;
            notebook.modify_style (rcstyle);
            Gtk.rc_parse_string (style_fixup);
#endif
            notify["index"].connect (index_changed);
            notify["tab"].connect (tab_changed);
            notify["labels-visible"].connect (labels_visible_changed);
            notify["close-buttons-visible"].connect (close_buttons_visible_changed);
            notify["close-buttons-left"].connect (close_buttons_left_changed);

            notebook.size_allocate.connect (size_allocated);
            notebook.switch_page.connect (page_switched);
            notebook.page_reordered.connect (page_moved);
            notebook.create_window.connect (window_created); 
            add_tab_btn = new Gtk.Button ();
            add_tab_btn.tooltip_text = _("Open a new tab");
            add_tab_btn.relief = Gtk.ReliefStyle.NONE;
            add_tab_btn.add (new Gtk.Image.from_gicon (new ThemedIcon.with_default_fallbacks ("cdos-add"), Gtk.IconSize.MENU));    //zgh 20150409  /*"gtk-add"*//*"tab-new-symbolic"*/ 
            add_tab_btn.show_all ();
           // notebook.set_action_widget (add_tab_btn, Gtk.PackType.START);//modified by wangyl in 2015.5.18
           //modified by wangyl in 2015.5.18 start
            notebook.add(add_tab_btn);
	         Gtk.Widget  page = notebook.get_nth_page(-1);
	         notebook.set_tab_label(page, add_tab_btn);
          //modified by wangyl in 2015.5.18  end
            add_tab_btn.clicked.connect (()=>{
                new_tab ();
            });
            notebook.button_press_event.connect (button_pressed);
        }
     

        void take_incoming_uris (Gtk.Widget widget) {
            Gtk.drag_dest_set (widget, Gtk.DestDefaults.ALL, (Gtk.TargetEntry[])null, Gdk.DragAction.COPY);
            Gtk.drag_dest_add_text_targets (widget);
            Gtk.drag_dest_add_uri_targets (widget);
            widget.drag_drop.connect (uri_dropped);
            widget.drag_data_received.connect (uri_received);
        }

        bool uri_dropped (Gtk.Widget widget, Gdk.DragContext context, int x, int y, uint timestamp) {
            Gtk.drag_finish (context, false, false, timestamp);
            return true;
        }

        void uri_received (Gtk.Widget widget, Gdk.DragContext context, int x, int y, Gtk.SelectionData data, uint ttype, uint timestamp) {
            string[] uri = data.get_uris ();
            string drag_uri = uri != null ? uri[0] : data.get_text ();
            Midori.Tab drag_tab;
            if (widget is Tally)
                drag_tab = (widget as Tally).tab;
            else {
                new_tab ();
                // Browser will have focussed the new tab
                drag_tab = tab;
            }
            drag_tab.web_view.load_uri (drag_uri);
        }


        ~Notebook () {
            notebook.size_allocate.disconnect (size_allocated);
            notebook.switch_page.disconnect (page_switched);
            notebook.page_reordered.disconnect (page_moved);
            notebook.create_window.disconnect (window_created);
        }

        /* Since: 0.5.8 */
        public ContextAction get_context_action () {
            var menu = new Midori.ContextAction ("NotebookContextMenu", null, null, null);
#if 0     //modified by wangyl in 2015.5.18
            uint counter = 0;
            foreach (var child in notebook.get_children ()) {
                var tab = child as Midori.Tab;
                var tally = notebook.get_tab_label (tab) as Tally;
                var action = new Midori.ContextAction.escaped ("Tab%u".printf (counter), tally.label.label, null, null);
                action.gicon = tally.icon.gicon;
                action.activate.connect (()=>{
                    notebook.set_current_page (notebook.page_num (tab));
                });
                menu.add (action);
                counter++;
            }
            context_menu (menu);
#else
        int counter = 0;
        foreach (var child in notebook.get_children ()) {
            if(notebook.get_n_pages() == (counter + 1))
                break;
            var tab = child as Midori.Tab;
            var tally = notebook.get_tab_label (tab) as Tally;
            var action = new Midori.ContextAction.escaped ("Tab%u".printf (counter), tally.label.label, null, null);
            action.gicon = tally.icon.gicon;
            action.activate.connect (()=>{
                notebook.set_current_page (notebook.page_num (tab));
            });
            menu.add (action);
            counter++;
        }
        context_menu (menu);
#endif            
            return menu;
        }
     bool    is_double_clicked_arrow(Gdk.EventButton event) {
          Gtk.Allocation size;
          Gtk.Allocation btn_size;
          notebook.get_allocation (out size);
          add_tab_btn.get_allocation (out btn_size);
          if((event.x > 0 && event.x < 21)||(event.x > size.width-btn_size.width-21 && event.x < size.width-btn_size.width))
            return true;

          return false;
        }
 bool button_pressed (Gdk.EventButton event) {
            /* Propagate events in logical label area */   
#if 0//modified by wangyl in 2015.5.18
            foreach (var child in notebook.get_children ()) {
                var tally = notebook.get_tab_label (tab) as Tally;
                Gtk.Allocation size;
                tally.get_allocation (out size);
                if (tally.get_mapped ()
                 && event.x_root >= size.x
                 && event.x_root <= (size.x + size.width)) {
                    tally.button_press_event (event);
                    return true;
         }
            }
#else
 	int counter = 0;
            var num = notebook.page_num (tab);
            double max_size = 0.0;
	foreach (var child in notebook.get_children ()) {
                if(notebook.get_n_pages() == (counter + 1)) {
                    var btn = notebook.get_tab_label(child);
                    Gtk.Allocation size;
                    btn.get_allocation (out size); 
                    max_size = size.x;
                    break;
                }    
                var tally = notebook.get_tab_label (tab) as Tally;
                Gtk.Allocation size;
                tally.get_allocation (out size); 
                counter++;
            }
#endif
            if (event.type == Gdk.EventType.2BUTTON_PRESS && event.button == 1 
             || event.button == 2) {
                if(btn_end == true && is_double_clicked_arrow(event) == true )return false;//modified by wangyl in 2015.5.18
               // new_tab ();  //modified by wangyl in 2015.5.18
                return true;
            }
            else if (event.button == 3) {
                var menu = get_context_action ();
                var popup = menu.create_menu (null, false);
                popup.show ();
                popup.attach_to_widget (this, null);
                popup.popup (null, null, null, event.button, event.time); 
                return true;
            }
            var c_page = notebook.get_current_page();
            var n_page = notebook.get_n_pages(); 
            if(event.button == 1 && event.x > max_size -4 && event.x < (max_size + 24)) {
                new_tab();
                notebook.set_current_page(notebook.get_n_pages() - 2);
            }     
            return false;
        }

        public void insert (Midori.Tab tab, int index) {
            var tally = new Tally (tab);
            tally.close_button_left = close_buttons_left;
            tally.close_button_visible = close_buttons_visible;

            tally.loc_simbo_button_left = loc_simbo_buttons_left;							//lxx, 20150129
            tally.loc_simbo_button_visible = loc_simbo_buttons_visible;					//lxx, 20150129

            tally.button_press_event.connect (tab_button_pressed);
            tally.show ();
            tally.set_size_request (tab.minimized ? -1 : last_tab_size, -1);
            take_incoming_uris (tally);

            /* Minimum requirements for any tab */
            tab.can_focus = tab.visible = true;
            notebook.insert_page (tab, tally, index);
            notebook.set_tab_reorderable (tab, true);
            notebook.set_tab_detachable (tab, true);
            tab.destroy.connect (tab_removed);
            tab.notify["minimized"].connect (tab_minimized);
            count++;
            tab.ref ();
            relayout ();
        }

        void tab_removed () {  
            var c_page = notebook.get_current_page();
            var n_page = notebook.get_n_pages();  
              if(btn_end == false && c_page+1 == n_page) notebook.set_current_page(c_page-1);
            count--;
            if (count > 0)
                relayout ();
           
        }

        void relayout () {
            Gtk.Allocation size;
            notebook.get_allocation (out size);
            resize (size.width);  
        }

        /* Since: 0.5.8 */
        public ContextAction get_tab_context_action (Midori.Tab tab) {
            var page_n = notebook.get_n_pages();
            var menu = new Midori.ContextAction ("TabContextMenu", null, null, null);
            tab_context_menu (tab, menu);
            var action_window = new Midori.ContextAction ("TabWindowNew", _("Open in New _Window"), null, "window-new");
            action_window.activate.connect (()=>{
                tab_detached (tab, 128, 128);
            });
            menu.add (action_window);
            var action_minimize = new Midori.ContextAction ("TabMinimize", tab.minimized ? _("Show Tab _Label") : _("Show Tab _Icon Only"), null, null);
            action_minimize.activate.connect (()=>{
                tab.minimized = !tab.minimized;
            });
            menu.add (action_minimize);
            var action_right = new Midori.ContextAction ("TabCloseRight", ngettext ("Close Tab to the R_ight", "Close Tabs to the R_ight", count - 1), null, null);
            //action_right.sensitive = count > 1;
            if(btn_end == false && notebook.page_num (tab) + 2 == page_n)action_right.set_sensitive(false);
            if(btn_end == true && notebook.page_num (tab) + 1 == page_n)action_right.set_sensitive(false);
            action_right.activate.connect (()=>{
                bool found_tab = false;
                int counter = 0; 
                foreach (var child in notebook.get_children ()) {
                    if (found_tab){
                         if(btn_end == false && page_n == (counter + 1))break;
                          if(btn_end == true && page_n == counter )break;
                        child.destroy ();  
                        }
                    else
                        found_tab = child == tab;
                    counter++;
                }
            });
            menu.add (action_right);
            var action_other = new Midori.ContextAction ("TabCloseOther", ngettext ("Close Ot_her Tab", "Close Ot_her Tabs", count - 1), null, null);
            action_other.sensitive = count > 1;
            action_other.activate.connect (()=>{
                int counter = 0; 
                foreach (var child in notebook.get_children ()){
                 if( btn_end == false &&page_n== (counter + 1))break;
                 if( btn_end == true &&page_n== counter )break;
                    if (child != tab)
                        child.destroy ();
                        counter ++;
                    }
            });
            menu.add (action_other);
            var action_close = new Midori.ContextAction ("TabClose", null, null, Gtk.STOCK_CLOSE);
            action_close.activate.connect (()=>{
                tab.destroy ();
            });
            menu.add (action_close);
            return menu;
         }

        bool tab_button_pressed (Gtk.Widget label, Gdk.EventButton event) {
            Tally tally = label as Tally;
        
            if (event.type == Gdk.EventType.2BUTTON_PRESS && event.button == 1 
             || event.button == 2) {
              //  tally.tab.destroy ();//modified by wangyl in 2015.5.18
            }
                if (event.button == 1  ) {
                /* Leave switching and dragging up to the notebook */
                return false;
            }
            else if (event.button == 3) {
                var menu = get_tab_context_action (tally.tab);
                var popup = menu.create_menu (null, false);
                popup.show ();
                popup.attach_to_widget (this, null);
                popup.popup (null, null, null, event.button, event.time);
            }
            return true;
        }
        
        public void move (Midori.Tab tab, int index) {
            notebook.reorder_child (tab, index);
        }

        /* Chain up drawing manually to circumvent parent checks */
#if HAVE_GTK3

        public override bool draw (Cairo.Context cr) {
            notebook.draw (cr);
            return true;
        }
#else
        public override bool expose_event (Gdk.EventExpose event) {
            notebook.expose_event (event);
            return true;
        }
#endif

        public override void forall_internal (bool include_internal, Gtk.Callback callback) {
        
            if (include_internal)
                callback (notebook);
 #if 0//modified by wangyl in 2015.5.18
            foreach (var child in notebook.get_children ())
                callback (child);
#else 
	 int counter = 0;
            foreach (var child in notebook.get_children ()) {
                   if(notebook.get_n_pages() == (counter + 1))
                    break;
                callback (child);
                counter++;
                 }
#endif
        }

        /* Can't override Gtk.Container.remove because it checks the parent */
        public new void remove (Midori.Tab tab) {
            return_if_fail (notebook.get_children ().find (tab) != null);
            notebook.remove (tab);
            tab.destroy.disconnect (tab_removed);
            tab.notify["minimized"].disconnect (tab_minimized);
            tab_removed ();
            tab.unref ();
        }

        void tab_minimized (GLib.ParamSpec pspec) {
            var tally = notebook.get_tab_label (tab) as Tally;
            tally.set_size_request (tab.minimized ? -1 : last_tab_size, -1);
        }

        public Midori.Tab get_nth_tab (int index) {
            return notebook.get_nth_page (index) as Midori.Tab;
        }

        public int get_tab_index (Midori.Tab tab) {
            return notebook.page_num (tab);
        }

        void index_changed (GLib.ParamSpec pspec) {
            notebook.set_current_page (index);
        }

        void tab_changed (GLib.ParamSpec pspec) {
            notebook.set_current_page (notebook.page_num (tab));
        }

        void labels_visible_changed (GLib.ParamSpec pspec) {
            notebook.show_tabs = labels_visible;
        }

        void close_buttons_visible_changed (GLib.ParamSpec pspec) {
            int counter = 0;
            foreach (var child in notebook.get_children ()) {
                 if(notebook.get_n_pages() == (counter + 1))
                    break;
                var tally = notebook.get_tab_label (child) as Tally;
                tally.close_button_visible = close_buttons_visible;
                counter++;
            }
        }

        void close_buttons_left_changed (GLib.ParamSpec pspec) {
           int counter = 0;
            foreach (var child in notebook.get_children ()) {
                 if(notebook.get_n_pages() == (counter + 1))
                    break;
                var tally = notebook.get_tab_label (child) as Tally;
                tally.close_button_left = close_buttons_left;
                counter++;
            }
        }
 //modified by wangyl in 2015.5.18 start 
int  is_arrrow_appear(Gtk.Allocation allocation,int page_n)
{
    Gtk.Allocation size; 
    Gtk.Allocation tab_size, btn_size,page_size,tab_size1;  
    int counter=0,total_size=10;
    var c_page = notebook.get_current_page();
    var n_page = notebook.get_n_pages();  
    add_tab_btn.get_allocation (out btn_size);   
    Gtk.Widget tab_label = notebook.get_tab_label (tab);
    tab_label .get_allocation (out tab_size); 
    foreach (var child in notebook.get_children ()) {
        if(btn_end == false && page_n == (counter + 1))
			 break;
			var tab1 = child as Midori.Tab;
			Gtk.Widget tab_label1 = notebook.get_tab_label (tab1);
			tab_label1 .get_allocation (out tab_size1); 

			total_size+=tab_size1.width + 7;
			counter++;
                }
    if(btn_end == false)total_size+= btn_size.width +7+10;
    else if (btn_end == true)total_size+= btn_size.width +8+32;
    if(btn_end == false && total_size>= allocation.width)return 0;
    if(btn_end == true && total_size< allocation.width)return -1;
    return 1;         
}
 
 //modified by wangyl in 2015.5.18 end
#if HAVE_GTK3
        void size_allocated (Gtk.Allocation allocation) {
        
#else
        void size_allocated (Gdk.Rectangle allocation) {
#endif
  
  //modified by wangyl in 2015.5.18 start 
             Gtk.Allocation tab_size, btn_size; 
            var page_n = notebook.get_n_pages();
      
           have_arrow = is_arrrow_appear(allocation,page_n);   
           if(have_arrow == 0){
				notebook.remove_page(page_n -1);
				notebook.set_action_widget(add_tab_btn, Gtk.PackType.END);
				btn_end  = true;
				}
           if(have_arrow == -1){
            notebook.set_action_widget(null, Gtk.PackType.END);
            notebook.add(add_tab_btn);
            Gtk.Widget last_page = notebook.get_nth_page(-1);
            notebook.set_tab_label(last_page, add_tab_btn);
            btn_end  = false;         
                   		} 
 //modified by wangyl in 2015.5.18 end 
            if (labels_visible && count > 0)
                resize (allocation.width);
        }

#if HAVE_GTK3
        void page_switched (Gtk.Widget new_tab_s, uint new_index) {
#else
        void page_switched (Gtk.NotebookPage new_tab_s, uint new_index) {
#endif 
            tab_switched (previous, new_tab_s as Tab);
            previous = (Midori.Tab)new_tab_s;
            notify["index"].disconnect (index_changed);
            notify["tab"].disconnect (tab_changed);
            index = (int)new_index;    
            tab = (Midori.Tab)new_tab_s; 
            notify["index"].connect (index_changed);
            notify["tab"].connect (tab_changed);            
        }

        void page_moved (Gtk.Widget moving_tab, uint new_index) {
            
            tab_moved (moving_tab as Midori.Tab, new_index);
            /* Indices change, current tab is not in the same position */
            notify["index"].disconnect (index_changed);
            index = (int)get_tab_index (tab) - 1 ;
            notify["index"].connect (index_changed);
//modified by wangyl in 2015.5.18 start
            int page_n = notebook.get_n_pages();
          
            if(page_n == new_index + 1) {
                notebook.remove_page(page_n - 2);
                notebook.add(add_tab_btn);
                Gtk.Widget page = notebook.get_nth_page(-1);
                notebook.set_tab_label(page, add_tab_btn);
            }
 //modified by wangyl in 2015.5.18 end 
        }

        unowned Gtk.Notebook window_created (Gtk.Widget tab, int x, int y) {
            tab_detached (tab as Tab, x, y);
            /* The API allows now, the cast is due to bindings not having ? */
            return (Gtk.Notebook)null;
        }

        void resize (int new_size) {
            int n = int.max (1, (int)count);
            new_size /= n;
            int icon_size = 16;
            Gtk.icon_size_lookup_for_settings (get_settings (),
                Gtk.IconSize.MENU, out icon_size, null);
            int max_size = 150;
            int min_size = icon_size;
            if (close_buttons_visible)
                min_size += icon_size;
            new_size = new_size.clamp (min_size, max_size);
            if ((new_size - last_tab_size).abs () < 3)
                return;     
            last_tab_size = new_size;
            int counter = 0;
            foreach (var child in notebook.get_children ()) {
                  if(notebook.get_n_pages() == (counter + 1))
                    break;
                var tab = child as Midori.Tab;
                var tally = notebook.get_tab_label (child) as Tally;
                tally.set_size_request (tab.minimized ? -1 : last_tab_size, -1);
                counter++;
            }
        }
    }
}


