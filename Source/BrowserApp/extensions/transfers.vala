/*
   Copyright (C) 2009-2013 Christian Dywan <christian@twotoasts.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   See the file COPYING for the full license text.
*/

namespace Gtk {
    extern static void widget_size_request (Gtk.Widget widget, out Gtk.Requisition requisition);
}

namespace Sokoke {
    extern static void widget_get_text_size (Gtk.Widget widget, string sample, out int width, out int height);
}

namespace Transfers {
    private class Transfer : GLib.Object {
        internal WebKit.Download download;
        internal string crtime;
        internal int64 crtime_i;
        internal string filename;
        internal string filesize;
        internal string uri;
        internal string content_type;
        internal int64 download_progress;
        internal string destination;

        internal signal void changed ();
        internal signal void remove ();
        internal signal void removed ();

        internal int action { get {
            return Midori.Download.get_type (download);
        } }
        internal double progress { get {
            return Midori.Download.get_progress (download);
        } }
#if HAVE_WEBKIT2
        public bool succeeded { get; protected set; default = true; }
        public bool finished { get; protected set; default = true; }

#else
        internal bool succeeded { get {
            return download.status == WebKit.DownloadStatus.FINISHED;
        } }
        internal bool finished { get {
            return Midori.Download.is_finished (download);
        } }
        internal string destination { get {
            return download.destination_uri;
        } }
#endif
  
        internal void update_database()
        {
            Midori.Database database;
            string sqlcmd = "UPDATE `download` SET file_size = :file_size, download_progress = :download_progress WHERE name = :filename";
            
            try {
                    database = new Midori.Database ("download.db");
                } catch (Midori.DatabaseError schema_error) {
                    error (schema_error.message);
                }

            try {
                    database.prepare (sqlcmd,
                        ":file_size", typeof (string), this.filesize,
                        ":download_progress", typeof (int64), this.download_progress,
                        ":filename", typeof(string), this.filename).exec();
                } catch (Error error) {
                    critical (_("Failed to update database: %s"), error.message);
                }
        }

        internal void set_download(WebKit.Download download, bool is_browser_private_model)
        {
            this.download = download;
            this.destination = download.destination;
            succeeded = finished = false;

            #if HAVE_WEBKIT2
            download.notify["estimated-progress"].connect (transfer_changed);
            download.finished.connect (() => {
                this.filename = Midori.Download.get_basename_for_display (download.destination);
                this.filesize = Midori.Download.get_size (download);
                this.download_progress = (int64)(this.progress * 100);
                if (!is_browser_private_model)
                    {
                    //zgh 更新数据库
                    update_database();
                    }
            succeeded = finished = true;
            changed ();
            });
            download.failed.connect (() => {
                succeeded = false;
                finished = true;
                changed ();
            });
            #else
            download.notify["status"].connect (transfer_changed);
            download.notify["progress"].connect (transfer_changed);
            #endif
        }
        internal Transfer () {
            this.download = null;
            GLib.DateTime time = new DateTime.now_local ();
            this.crtime_i = time.to_unix();
            this.filename = null;
            this.filesize = null;
            this.uri = null;
            this.content_type = null;
            this.crtime = time.format("%Y-%m-%d %H:%M:%S");
        }
        void transfer_changed (GLib.ParamSpec pspec) {
            changed ();
        }
    }

    static bool pending_transfers (Katze.Array array) {
        foreach (GLib.Object item in array.get_items ()) {
            var transfer = item as Transfer;
            if (!transfer.finished)
                return true;
        }
        return false;
    }

    private class Sidebar : Gtk.VBox, Midori.Viewable {
        Gtk.Toolbar? toolbar = null;
        Gtk.ToolButton new_dl;
        Gtk.ToolButton start_dl;
        //Gtk.ToolButton stop_dl;
        Gtk.ToolButton cancel_dl;
        Gtk.ToolButton open_file;
        Gtk.ToolButton open_dir;
        Gtk.ToolButton clear;
        Gtk.ListStore store = new Gtk.ListStore (1, typeof (Transfer));
        Gtk.TreeView treeview;
        Katze.Array array;
        
//        Midori.Database database;

        public unowned string get_stock_id () {
            return Midori.Stock.TRANSFER;
        }

        public unowned string get_label () {
            return _("Transfers");
        }

        public Gtk.Widget get_toolbar () {
            if (toolbar == null) {
                toolbar = new Gtk.Toolbar ();
                toolbar.set_icon_size (Gtk.IconSize.BUTTON);
                toolbar.insert (new Gtk.ToolItem (), -1);
                var separator = new Gtk.SeparatorToolItem ();
                separator.draw = false;
                separator.set_expand (true);
                toolbar.insert (separator, -1);
                clear = new Gtk.ToolButton.from_stock (Gtk.STOCK_CLEAR);
                //clear.label = _("Clear All");
		clear.label = "清空";
                clear.is_important = true;
                clear.set_tooltip_text("清除已结束的下载任务");
                clear.clicked.connect (clear_clicked);
                clear.sensitive = !array.is_empty ();
                toolbar.insert (clear, 0);
                //toolbar.insert (separator, 0);//comment by zlf

                open_dir = new Gtk.ToolButton.from_stock (Gtk.STOCK_DIRECTORY);
                open_dir.label = "打开目录";
                open_dir.is_important = true;
                open_dir.clicked.connect (open_dir_clicked);
                open_dir.set_tooltip_text("打开目录");
                open_dir.sensitive = false;
                toolbar.insert (open_dir, 0);
                //toolbar.insert (separator, 0);      
                
                open_file = new Gtk.ToolButton.from_stock (Gtk.STOCK_NEW);//GTK_STOCK_FILE
                open_file.label = "打开文件";//_("OpenFile");
                open_file.is_important = true;
                open_file.clicked.connect (open_file_clicked);
		open_file.set_tooltip_text("打开文件");
		open_file.sensitive = false;
                toolbar.insert (open_file, 0);
                //toolbar.insert (separator, 0);      
                
                cancel_dl = new Gtk.ToolButton.from_stock (Gtk.STOCK_CANCEL);
                cancel_dl.label = "删除记录";
                cancel_dl.is_important = true;
                cancel_dl.clicked.connect (cancel_dl_clicked);
                cancel_dl.set_tooltip_text("删除记录");
	     cancel_dl.sensitive = false;
                toolbar.insert (cancel_dl, 0);
                //toolbar.insert (separator, 0);      
                
                start_dl = new Gtk.ToolButton.from_stock (Gtk.STOCK_MEDIA_PLAY);
                start_dl.label = "重新下载";
                start_dl.is_important = true;
                start_dl.clicked.connect (start_dl_clicked);
		start_dl.set_tooltip_text("重新下载");
		start_dl.sensitive = false;
                toolbar.insert (start_dl, 0);
                //toolbar.insert (separator, 0);      
                
                new_dl = new Gtk.ToolButton.from_stock (Gtk.STOCK_ADD);
                new_dl.label = "新建下载";
                new_dl.is_important = true;
                new_dl.clicked.connect (new_dl_clicked);
		 new_dl.set_tooltip_text("新建下载");
                toolbar.insert (new_dl, 0);
                
                toolbar.show_all ();
            }
            return toolbar;
        }

        void clear_clicked () {
            stdout.printf("clear_dir_clicked\n");
            cancel_dl.label = "删除记录";
            cancel_dl.set_tooltip_text("删除记录");
            cancel_dl.sensitive = false;
            foreach (GLib.Object item in array.get_items ()) {
                var transfer = item as Transfer;
                if (transfer.finished)
                    transfer.remove ();
            }
        }

        void open_dir_clicked () {
            stdout.printf("open_dir_clicked\n");
            Gtk.TreeIter iter;
            if (treeview.get_selection ().get_selected (null, out iter)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);
                var folder = GLib.File.new_for_uri (transfer.destination);
                string[] argv = {"xdg-open", folder.get_parent ().get_uri (),null};
                GLib.Process.spawn_async(null,argv,null,
                                        GLib.SpawnFlags.SEARCH_PATH
                                        |GLib.SpawnFlags.STDOUT_TO_DEV_NULL
                                        |GLib.SpawnFlags.STDERR_TO_DEV_NULL
                                        |GLib.SpawnFlags.STDERR_TO_DEV_NULL,
                                        null,null);
                                }

        }

        void open_file_clicked () {
            stdout.printf("open_file_clicked\n");
            Gtk.TreeIter iter;
            if (treeview.get_selection ().get_selected (null, out iter)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);
                if (transfer.finished) {
                    try {
                        //Midori.Download.open (transfer.download, treeview);   //zgh 打开文件，修改
                        (Midori.Browser.get_for_widget (this).tab as Midori.Tab).open_uri (transfer.destination);
                        
                        //zgh 工具栏上图标变化
                        var action_group = (Midori.Browser.get_for_widget (this)).get_action_group();
                        var download_action = action_group.get_action("DownloadDialog");
                        download_action.set("stock-id", "gtk-go-down", null);
                    } catch (Error error_open) {
                        GLib.warning (_("Failed to open download: %s"), error_open.message);
                    }
                }
            }
        }

        void cancel_dl_clicked () {
            stdout.printf("cancel_clicked\n");

            Gtk.TreeIter iter;
            if (treeview.get_selection ().get_selected (null, out iter)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);

                if (transfer.finished)
                    transfer.remove ();
                else {
                    var dialog = new Gtk.MessageDialog (null,
                        Gtk.DialogFlags.DESTROY_WITH_PARENT,
                        Gtk.MessageType.WARNING, Gtk.ButtonsType.NONE,
                        "是否暂停下载?");
                    dialog.title = "警告";
                    dialog.add_buttons ("停止下载", Gtk.ResponseType.CANCEL, //Gtk.STOCK_ADD
                                           "取消", Gtk.ResponseType.ACCEPT);
                    bool cancel = dialog.run () != Gtk.ResponseType.ACCEPT;
                        dialog.destroy ();
                    if(cancel)
                    {
                        try {
                            if (Midori.Download.action_clear (transfer.download, treeview))
                                transfer.remove ();
                        } catch (Error error) {
                            // Failure to open is the only known possibility here
                            GLib.warning (_("Failed to open download: %s"), error.message);
                        }
                    }
                }
            }
        }

        //array = new Katze.Array (typeof (Transfer));
        void start_dl_clicked () {
            stdout.printf("start_dl_clicked\n");
            Gtk.TreeIter iter;
            if (treeview.get_selection ().get_selected (null, out iter)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);

                var dialog = new Gtk.MessageDialog (null,
                    Gtk.DialogFlags.DESTROY_WITH_PARENT,
                    Gtk.MessageType.WARNING, Gtk.ButtonsType.NONE,
                    "是否重新下载?");
                dialog.title = "警告";
                dialog.add_buttons ("重新下载", Gtk.ResponseType.CANCEL,
                                   "取消", Gtk.ResponseType.ACCEPT);
                bool cancel = dialog.run () != Gtk.ResponseType.ACCEPT;
                dialog.destroy ();
                if(cancel)
                {
                    transfer.remove ();
                    if (transfer.download != null)
                        Midori.Download.re_download(transfer.download);
                    else{
                        WebKit.WebContext m_webContext = WebKit.WebContext.get_default();
                        WebKit.Download downloadt = m_webContext.download_uri(transfer.uri);
                        Midori.Download.set_filename(downloadt, transfer.filename);
                        downloadt.get_request();
                    }
                }
            }            
        }

        void new_dl_clicked () {
            stdout.printf("new_dl_clicked\n");
            Midori.Download.new_download();

        }
        
        void cursor_changed () {
            stdout.printf("cursor_changed\n");
            Gtk.TreeIter iter;
            if (treeview.get_selection ().get_selected (null, out iter)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);
                if (transfer.finished) {
                    cancel_dl.label = "删除记录";
                    cancel_dl.set_tooltip_text("删除记录");
                
                    if (transfer.finished && transfer.download_progress == 100) {
                        open_file.sensitive = true;
                    }
                }
                else{
                    open_file.sensitive = false;
                    cancel_dl.label = "取消下载";
                    cancel_dl.set_tooltip_text("取消下载");
                }
            }
            open_dir.sensitive = true;
//            open_file.sensitive = true;
            cancel_dl.sensitive = true;
            start_dl.sensitive = true;
        }

        public Sidebar (Katze.Array array, bool is_browser_private_model) {
            //Gtk.Widget 
            var scrolled = new Gtk.ScrolledWindow(null, null);
            Gtk.TreeViewColumn column;

            treeview = new Gtk.TreeView.with_model (store);
            treeview.headers_visible = true;

            store.set_sort_column_id (0, Gtk.SortType.ASCENDING);
            store.set_sort_func (0, tree_sort_func);

            //gtk_tree_view_column_set_max_width (GtkTreeViewColumn *tree_column, gint               max_width)


            column = new Gtk.TreeViewColumn ();
            Gtk.CellRendererPixbuf renderer_icon = new Gtk.CellRendererPixbuf ();
            column.pack_start (renderer_icon, false);
            column.set_cell_data_func (renderer_icon, on_render_icon);
            treeview.append_column (column);

            column = new Gtk.TreeViewColumn ();
            //gtk_tree_view_column_set_title
            column.set_title("文件名");
            //column.set_max_width(150);
            column.set_sizing (Gtk.TreeViewColumnSizing.AUTOSIZE);
            Gtk.CellRendererProgress renderer_progress = new Gtk.CellRendererProgress ();
            column.pack_start (renderer_progress, true);
            column.set_expand (false);
            column.set_cell_data_func (renderer_progress, on_render_text);
            treeview.append_column (column);

            column = new Gtk.TreeViewColumn ();
            column.set_title("大小");
            column.set_sizing (Gtk.TreeViewColumnSizing.GROW_ONLY); //GTK_TREE_VIEW_COLUMN_GROW_ONLY
            Gtk.CellRendererText size = new Gtk.CellRendererText ();
            column.pack_start (size, true);
            column.set_expand (false);
            column.set_cell_data_func (size, on_render_size);
            treeview.append_column (column);

            column = new Gtk.TreeViewColumn ();
            column.set_title("剩余时间");
            column.set_sizing (Gtk.TreeViewColumnSizing.AUTOSIZE);
            Gtk.CellRendererText remaining = new Gtk.CellRendererText ();
            column.pack_start (remaining, true);
            column.set_expand (false);
            column.set_cell_data_func (remaining, on_render_remaining);
            treeview.append_column (column);

            column = new Gtk.TreeViewColumn ();
            column.set_title("创建时间");
            column.set_sizing (Gtk.TreeViewColumnSizing.AUTOSIZE);
            Gtk.CellRendererText creatime = new Gtk.CellRendererText ();
            column.pack_start (creatime, true);
            column.set_expand (false);
            //gtk_cell_layout_default_set_cell_data_func
            column.set_cell_data_func (creatime, on_render_creatime);
            treeview.append_column (column);

            column = new Gtk.TreeViewColumn ();
            column.set_title("网址");
            column.set_max_width(300);
            column.set_sizing (Gtk.TreeViewColumnSizing.AUTOSIZE);
            Gtk.CellRendererText website = new Gtk.CellRendererText ();
            column.pack_start (website, true);
            column.set_expand (false);
            column.set_cell_data_func (website, on_render_website);
            treeview.append_column (column);

            /*
            treeview = new Gtk.TreeView.with_model (store);
            column = new Gtk.TreeViewColumn ();
            Gtk.CellRendererPixbuf renderer_button = new Gtk.CellRendererPixbuf ();
            column.pack_start (renderer_button, false);
            column.set_cell_data_func (renderer_button, on_render_button);
            treeview.append_column (column);
            */
            
            //zgh add signal 
            treeview.cursor_changed.connect(cursor_changed);

            treeview.row_activated.connect (row_activated);
            treeview.button_release_event.connect (button_released);
            treeview.popup_menu.connect (menu_popup);
            treeview.show ();
            pack_start (treeview, true, true, 0);
            
            //(scrolled.get_content_area () as Gtk.Box).pack_start (treeview, false, false, 0);
            //scrolled.get_content_area ().show_all ();
            /*
            void GTK_TREE_VIEW_COLUMN_FIXED
gtk_tree_view_column_set_sizing (GtkTreeViewColumn       *tree_column,
                                 GtkTreeViewColumnSizing  type)
                        Gtk.TreeView treeview;GTK_POLICY_AUTOMATIC Gtk.PolicyType.AUTOMATIC
            GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);

            download_list_init(dialog);
            gtk_container_add(GTK_CONTAINER(scrolledWindow), dialog->downloadList);
            gtk_widget_show(dialog->downloadList);

            gtk_box_pack_start(contentArea, scrolledWindow, TRUE, TRUE, 0);
            gtk_widget_show(scrolledWindow);
            */

            this.array = array;
            array.add_item.connect (transfer_added);
            array.remove_item.connect_after (transfer_removed);
            foreach (GLib.Object item in array.get_items ())
                transfer_added (item);
              /*  
            if (!is_browser_private_model)
            {
                try {
                    database = new Midori.Database ("download.db");
                } catch (Midori.DatabaseError schema_error) {
                    error (schema_error.message);
                }
            }
            */
        }

        void row_activated (Gtk.TreePath path, Gtk.TreeViewColumn column) {
            stdout.printf("row_activated\n");
            /*
            Gtk.TreeIter iter;
            if (store.get_iter (out iter, path)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);

                try {
                    if (Midori.Download.action_clear (transfer.download, treeview))
                        transfer.remove ();
                } catch (Error error) {
                    // Failure to open is the only known possibility here
                    GLib.warning (_("Failed to open download: %s"), error.message);
                }
            }
            */
        }

        bool button_released (Gdk.EventButton event) {
            if (event.button == 3)
                return show_popup_menu (event);
            return false;
        }

        bool menu_popup () {
            return show_popup_menu (null);
        }

        bool show_popup_menu (Gdk.EventButton? event) {
            Gtk.TreeIter iter;
            if (treeview.get_selection ().get_selected (null, out iter)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);

                var menu = new Gtk.Menu ();
                var menuitem = new Gtk.ImageMenuItem.from_stock (Gtk.STOCK_OPEN, null);
                menuitem.activate.connect (() => {
                    try {
//zgh                   Midori.Download.open (transfer.download, treeview);
                        (Midori.Browser.get_for_widget (this).tab as Midori.Tab).open_uri (transfer.download.destination);
                    } catch (Error error_open) {
                        GLib.warning (_("Failed to open download: %s"), error_open.message);
                    }
                });
                menuitem.sensitive = transfer.succeeded;
                menu.append (menuitem);
                menuitem = new Gtk.ImageMenuItem.with_mnemonic (_("Open Destination _Folder"));
                menuitem.image = new Gtk.Image.from_stock (Gtk.STOCK_DIRECTORY, Gtk.IconSize.MENU);
                menuitem.activate.connect (() => {
                    var folder = GLib.File.new_for_uri (transfer.destination);
//zgh                    (Midori.Browser.get_for_widget (this).tab as Midori.Tab).open_uri (folder.get_parent ().get_uri ());
                    string[] argv = {"xdg-open", folder.get_parent ().get_uri (),null};

                    GLib.Process.spawn_async(null,argv,null,
                                            GLib.SpawnFlags.SEARCH_PATH
                                            |GLib.SpawnFlags.STDOUT_TO_DEV_NULL
                                            |GLib.SpawnFlags.STDERR_TO_DEV_NULL
                                            |GLib.SpawnFlags.STDERR_TO_DEV_NULL,
                                            null,null);
                });
                menu.append (menuitem);
                menuitem = new Gtk.ImageMenuItem.with_mnemonic (_("Copy Link Loc_ation"));
                menuitem.activate.connect (() => {
                    string uri = transfer.destination;
                    get_clipboard (Gdk.SELECTION_PRIMARY).set_text (uri, -1);
                    get_clipboard (Gdk.SELECTION_CLIPBOARD).set_text (uri, -1);
                });
                menuitem.image = new Gtk.Image.from_stock (Gtk.STOCK_COPY, Gtk.IconSize.MENU);
                menu.append (menuitem);
                menu.show_all ();
                Katze.widget_popup (treeview, menu, null, Katze.MenuPos.CURSOR);

                return true;
            }
            return false;
        }

        int tree_sort_func (Gtk.TreeModel model, Gtk.TreeIter a, Gtk.TreeIter b) {
            Transfer transfer1, transfer2;
            model.get (a, 0, out transfer1);
            model.get (b, 0, out transfer2);
            return (transfer1.finished ? 1 : 0) - (transfer2.finished ? 1 : 0);
        }

        void transfer_changed () {
        //zgh 若选中的下载任务下载完成，打开文件按钮敏感性置为true
            Gtk.TreeIter iter;
            if (treeview.get_selection ().get_selected (null, out iter)) {
                Transfer transfer;
                store.get (iter, 0, out transfer);
                if(transfer.finished) {
                    cancel_dl.label = "删除记录";
                    cancel_dl.set_tooltip_text("删除记录");
                     if (transfer.download_progress == 100) {
                        open_file.sensitive = true;
                    }
                }
                else {
                    open_file.sensitive = false;
                }
            }
            
            treeview.queue_draw ();
        }

        void transfer_added (GLib.Object item) {
            var transfer = item as Transfer;
            Gtk.TreeIter iter;
            //store.append (out iter);
            store.insert (out iter, 0); //zgh 20150228 将新的下载任务添加到treeview的最上面
            store.set (iter, 0, transfer);
            transfer.changed.connect (transfer_changed);
            clear.sensitive = true;
        }

        void transfer_removed (GLib.Object item) {
            var transfer = item as Transfer;
            transfer.changed.disconnect (transfer_changed);
            Gtk.TreeIter iter;
            if (store.iter_children (out iter, null)) {
                do {
                    Transfer found;
                    store.get (iter, 0, out found);
                    if (transfer == found) {
                        store.remove (iter);
                        break;
                    }
                } while (store.iter_next (ref iter));
            }
            if (array.is_empty ())
                clear.sensitive = false;
                //zgh 若任务被清空，则按钮置灰
                open_dir.sensitive = false;
                open_file.sensitive = false;
                cancel_dl.sensitive = false;
                start_dl.sensitive = false;
                
                //zgh 工具栏上图标变化
                var action_group = (Midori.Browser.get_for_widget (this)).get_action_group();
                var download_action = action_group.get_action("DownloadDialog");
                download_action.set("stock-id", "gtk-go-down", null);
        }

        void on_render_icon (Gtk.CellLayout column, Gtk.CellRenderer renderer,
            Gtk.TreeModel model, Gtk.TreeIter iter) {

            Transfer transfer;
            string content_type;
            model.get (iter, 0, out transfer);
            if(transfer.download != null)
                content_type = Midori.Download.get_content_type (transfer.download, null);
            else
                content_type = transfer.content_type;
            var icon = GLib.ContentType.get_icon (content_type) as ThemedIcon;
            icon.append_name ("text-html");
            renderer.set ("gicon", icon,
                          //"stock-size", Gtk.IconSize.DND,
                          "stock-size", Gtk.IconSize.MENU,
                          "xpad", 1, "ypad", 12);
        }

        void on_render_text (Gtk.CellLayout column, Gtk.CellRenderer renderer,
            Gtk.TreeModel model, Gtk.TreeIter iter) {

            Transfer transfer;
            string tooltip;
            int progrs = 0;
            model.get (iter, 0, out transfer);
            if (transfer.download != null)
            {
                tooltip = Midori.Download.get_tooltip (transfer.download);
                progrs = (int)(transfer.progress * 100);
            }
            else
            {
                tooltip = transfer.filename;
                progrs = (int)transfer.download_progress;
            }
            renderer.set ("text", tooltip,
                            "value", progrs);
        }

        void on_render_size (Gtk.CellLayout column, Gtk.CellRenderer renderer,
            Gtk.TreeModel model, Gtk.TreeIter iter) {

            Transfer transfer;
            string size;
            model.get (iter, 0, out transfer);
            if (transfer.download != null)
                size = Midori.Download.get_size (transfer.download);
            else
                size = transfer.filesize;
            renderer.set ("text", size);
        }

        void on_render_remaining (Gtk.CellLayout column, Gtk.CellRenderer renderer,
            Gtk.TreeModel model, Gtk.TreeIter iter) {

            Transfer transfer;
            string remaining;
            model.get (iter, 0, out transfer);
            if (transfer.download != null)
                remaining = Midori.Download.get_remaining (transfer.download);
            else
                remaining = "0";
            renderer.set ("text", remaining);
        }

        void on_render_website (Gtk.CellLayout column, Gtk.CellRenderer renderer,
            Gtk.TreeModel model, Gtk.TreeIter iter) {

            Transfer transfer;
            string website;
            model.get (iter, 0, out transfer);
            if (transfer.download != null)
                website = Midori.Download.get_website (transfer.download);
            else
                website = transfer.uri;
            renderer.set ("text", website,
                            "ellipsize-set", true,
                            "ellipsize", Pango.EllipsizeMode.MIDDLE);
        }

        
        void on_render_creatime (Gtk.CellLayout column, Gtk.CellRenderer renderer,
            Gtk.TreeModel model, Gtk.TreeIter iter) {

            Transfer transfer;
            model.get (iter, 0, out transfer);
            renderer.set ("text", transfer.crtime);
        }

        /*
        void on_render_button (Gtk.CellLayout column, Gtk.CellRenderer renderer,
            Gtk.TreeModel model, Gtk.TreeIter iter) {

            Transfer transfer;
            model.get (iter, 0, out transfer);
            string stock_id = Midori.Download.action_stock_id (transfer.download);
            renderer.set ("stock-id", stock_id,
                          "stock-size", Gtk.IconSize.MENU);
        }
        */
    }

    private class TransferButton : Gtk.ToolItem {
        Transfer transfer;
        Gtk.ProgressBar progress;
        Gtk.Image icon;
        Gtk.Button button;

        public TransferButton (Transfer transfer) {
            this.transfer = transfer;

            var box = new Gtk.HBox (false, 0);
            progress = new Gtk.ProgressBar ();
#if HAVE_GTK3
            progress.show_text = true;
#endif
            progress.ellipsize = Pango.EllipsizeMode.MIDDLE;
            string filename = Midori.Download.get_basename_for_display (transfer.destination);
            
            progress.text = filename;
            int width;
            Sokoke.widget_get_text_size (progress, "M", out width, null);
            stdout.printf("TransferButton::filename[%s]-width[%d]\n", filename, width);
            progress.set_size_request (width * 10, 1);
            box.pack_start (progress, false, false, 0);

            icon = new Gtk.Image ();
            button = new Gtk.Button ();
            button.relief = Gtk.ReliefStyle.NONE;
            button.focus_on_click = false;
            button.clicked.connect (button_clicked);
            button.add (icon);
            box.pack_start (button, false, false, 0);

            add (box);
            show_all ();

            transfer.changed.connect (transfer_changed);
            transfer_changed ();
            transfer.removed.connect (transfer_removed);
        }

        void button_clicked () {
            try {
                if (Midori.Download.action_clear (transfer.download, button))
                    transfer.remove ();
            } catch (Error error) {
                // Failure to open is the only known possibility here
                GLib.warning (_("Failed to open download: %s"), error.message);
            }
        }

        void transfer_changed () {
            progress.fraction = Midori.Download.get_progress (transfer.download);
            progress.tooltip_text = Midori.Download.get_tooltip (transfer.download);
            string stock_id = Midori.Download.action_stock_id (transfer.download);
            icon.set_from_stock (stock_id, Gtk.IconSize.MENU);
        }

        void transfer_removed () {
            destroy ();
        }
    }

    private class Toolbar : Gtk.Toolbar {
        Katze.Array array;
        Gtk.ToolButton clear;

        void clear_clicked () {
            stdout.printf("clear_clicked 2\n");
            foreach (GLib.Object item in array.get_items ()) {
                var transfer = item as Transfer;
                if (transfer.finished)
                    array.remove_item (item);
            }
        }

        public Toolbar (Katze.Array array) {
            set_icon_size (Gtk.IconSize.BUTTON);
            set_style (Gtk.ToolbarStyle.BOTH_HORIZ);
            show_arrow = false;
// zgh
            clear = new Gtk.ToolButton.from_stock (Gtk.STOCK_CLEAR);
            clear.label = _("Clear All");
            clear.is_important = true;
            clear.clicked.connect (clear_clicked);
            clear.sensitive = !array.is_empty ();
            insert (clear, -1);
            clear.show ();
            clear.sensitive = false;

            this.array = array;
            array.add_item.connect (transfer_added);
            array.remove_item.connect_after (transfer_removed);
            foreach (GLib.Object item in array.get_items ())
                transfer_added (item);
        }

        void transfer_added (GLib.Object item) {
            var transfer = item as Transfer;
            /* Newest item on the left */
            insert (new TransferButton (transfer), 0);
            clear.sensitive = true;
            show ();

            Gtk.Requisition req;
            Gtk.widget_size_request (parent, out req);
            int reqwidth = req.width;
            int winwidth;
            (get_toplevel () as Gtk.Window).get_size (out winwidth, null);
            //if (reqwidth > winwidth)
                //clear_clicked ();
        }

        void transfer_removed (GLib.Object item) {
            clear.sensitive = pending_transfers (array);
            if (array.is_empty ())
                hide ();
        }
    }

    private class Manager : Midori.Extension {
        internal Katze.Array array;
        internal GLib.List<Gtk.Widget> widgets;
        internal GLib.List<string> notifications;
        internal uint notification_timeout;
        internal Midori.Database database;

        void download_added (WebKit.Download download) {

//            var transfer = new Transfer (download);
            var transfer = new Transfer ();
            transfer.set_download (download, get_app().settings.is_private);
            
            transfer.remove.connect (transfer_remove);
            transfer.changed.connect (transfer_changed);
            array.remove_item.connect (transfer_removed);
            array.add_item (transfer);
            if (!get_app().settings.is_private)
            {
                //zgh todo 写数据库，插入新的下载数据
                
    //            int64 create_time = transfer.get_createtime_i();
                string sqlcmd = "INSERT INTO `download` (`name`, `destination`, `file_size`, `uri`, `create_time`, `content_type`, `download_progress`) VALUES  (:name, :destination, :file_size, :uri, :create_time, :content_type, :download_progress);";
                
                try {
                        int64 download_progress = 0;
                        var statement = database.prepare (sqlcmd,
                            ":name", typeof (string), Midori.Download.get_basename_for_display (download.destination),
                            ":destination", typeof (string), transfer.destination,
                            ":file_size", typeof (string), "0",
                            ":uri", typeof (string), Midori.Download.get_website(download),
                            ":create_time", typeof (int64), transfer.crtime_i,
                            ":content_type", typeof (string), Midori.Download.get_content_type (transfer.download, null),
                            ":download_progress", typeof (int64), download_progress);
                        statement.exec ();
                    } catch (Error error) {
                        critical (_("Failed to update database: %s"), error.message);
                    }
                }
        }

        bool notification_timeout_triggered () {
            notification_timeout = 0;
            if (notifications.length () > 0) {
                string filename = notifications.nth_data(0);
                string msg;
                if (notifications.length () == 1)
                    msg = _("The file '<b>%s</b>' has been downloaded.").printf (filename);
                else
                    msg = _("'<b>%s</b>' and %d other files have been downloaded.").printf (filename, notifications.length ());
                get_app ().send_notification (_("Transfer completed"), msg);
                notifications = new GLib.List<string> ();
            }
            return false;
        }

        void transfer_changed (Transfer transfer) {
            if (transfer.succeeded) {
                /* FIXME: The following 2 blocks ought to be done in core */
                if (transfer.action == Midori.DownloadType.OPEN) {
                    try {
                        Midori.Download.action_clear (transfer.download, widgets.nth_data (0));
                        //if (Midori.Download.action_clear (transfer.download, widgets.nth_data (0)))
                          //  transfer.remove ();
                    } catch (Error error) {
                        // Failure to open is the only known possibility here
                        GLib.warning (_("Failed to open download: %s"), error.message);
                    }
                }

                string uri = transfer.destination;
                string filename = Midori.Download.get_basename_for_display (uri);
                var item = new Katze.Item ();
                item.uri = uri;
                item.name = filename;
                Midori.Browser.update_history (item, "download", "create");
                if (!Midori.Download.has_wrong_checksum (transfer.download))
                    Gtk.RecentManager.get_default ().add_item (uri);

                /*
                notifications.append (filename);
                if (notification_timeout == 0) {

                    notification_timeout_triggered ();
                    notification_timeout = Midori.Timeout.add_seconds (60, notification_timeout_triggered);
                }
                */
            }
        }

        void transfer_remove (Transfer transfer) {
            array.remove_item (transfer);
            
            if (!get_app().settings.is_private)
            {
                //zgh delete from database
                string sqlcmd = "DELETE FROM download WHERE destination = :destination";
                try{
                    var statement = database.prepare(sqlcmd,
                    ":destination", typeof (string), transfer.destination);
                    statement.exec();
                }catch (Error error){
                    critical(_("Failed to delete database: %s"), error.message);
                }
            }
            
        }

        void transfer_removed (GLib.Object item) {
            var transfer = item as Transfer;
            transfer.removed ();
        }

#if HAVE_GTK3
        bool browser_closed (Gtk.Widget widget, Gdk.EventAny event) {
#else
        bool browser_closed (Gtk.Widget widget, Gdk.Event event) {
#endif
            var app = get_app ();
            var browser = widget as Midori.Browser;
            if (1 == app.get_browsers ().length () && pending_transfers (array)) {
                var dialog = new Gtk.MessageDialog (browser,
                    Gtk.DialogFlags.DESTROY_WITH_PARENT,
                    Gtk.MessageType.WARNING, Gtk.ButtonsType.NONE,
                    _("Some files are being downloaded"));
                dialog.title = _("Some files are being downloaded");
                dialog.add_buttons (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                    _("_Quit Midori"), Gtk.ResponseType.ACCEPT);
                dialog.format_secondary_text (
                    _("The transfers will be cancelled if Midori quits."));
                bool cancel = dialog.run () != Gtk.ResponseType.ACCEPT;
                dialog.destroy ();
                return cancel;
            }
            return false;
        }

        void browser_added (Midori.Browser browser) {
            var viewable = new Sidebar (array, get_app().settings.is_private);
            viewable.show ();
            browser.panel.append_page (viewable);
            widgets.append (viewable);
/* zgh 删除在状态栏中的显示
            var toolbar = new Toolbar (array);
#if HAVE_GTK3
            browser.statusbar.pack_end (toolbar, false, false);
#else
            browser.statusbar.pack_start (toolbar, false, false);
#endif
            widgets.append (toolbar);
*/
            // TODO: popover
            // TODO: progress in dock item
            browser.add_download.connect (download_added);
            browser.clear_download.connect (clear_download);
            browser.delete_event.connect (browser_closed);
        }
        
        void clear_download (Gtk.Widget widget)
        {
            foreach (GLib.Object item in array.get_items ()) {
                var transfer = item as Transfer;
                if (transfer.finished)
                    transfer.remove ();
            }
        }
        
        void get_download_array()
        {
            string sqlcmd = "SELECT name, destination, file_size, uri, create_time, content_type, download_progress FROM download ORDER BY create_time ASC";
            
            try {
                    var statement = database.prepare (sqlcmd);
                    while (statement.step ()) {
                        var transfer = new Transfer ();
                        transfer.remove.connect (transfer_remove);
                        transfer.changed.connect (transfer_changed);
                        array.remove_item.connect (transfer_removed);
                        
                        transfer.filename = statement.get_string ("name");
                        transfer.destination = statement.get_string ("destination");
                        transfer.filesize = statement.get_string ("file_size");
                        transfer.uri = statement.get_string ("uri");
                        transfer.crtime_i = statement.get_int64 ("create_time");
                        transfer.crtime = new DateTime.from_unix_local (transfer.crtime_i).format("%Y-%m-%d %H:%M:%S");
                        transfer.content_type = statement.get_string("content_type");
                        transfer.download_progress = statement.get_int64 ("download_progress");

                        array.add_item (transfer);
                    }
                } catch (Error error) {
                    critical (_("Failed to select from database: %s"), error.message);
                }
        }

        void activated (Midori.App app) {
            array = new Katze.Array (typeof (Transfer));
            if (!app.settings.is_private)
            {
                try {
                    database = new Midori.Database ("download.db");
                } catch (Midori.DatabaseError schema_error) {
                    error (schema_error.message);
                }
            get_download_array();
            }
            widgets = new GLib.List<Gtk.Widget> ();
            notifications = new GLib.List<string> ();
            notification_timeout = 0;
            foreach (var browser in app.get_browsers ())
                browser_added (browser);
            app.add_browser.connect (browser_added);
        }

        void deactivated () {
            var app = get_app ();
            app.add_browser.disconnect (browser_added);
            foreach (var browser in app.get_browsers ()) {
                browser.add_download.disconnect (download_added);
                browser.delete_event.disconnect (browser_closed);
            }
            foreach (var widget in widgets)
                widget.destroy ();
            array.remove_item.disconnect (transfer_removed);
        }

        internal Manager () {
            GLib.Object (name: _("Transfer Manager"),
                         description: _("View downloaded files"),
                         version: "0.1" + Midori.VERSION_SUFFIX,
                         authors: "Christian Dywan <christian@twotoasts.de>");

            this.activate.connect (activated);
            this.deactivate.connect (deactivated);
        }
    }
}
 
public Midori.Extension extension_init () {
    return new Transfers.Manager ();
}

