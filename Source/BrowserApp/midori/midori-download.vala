/*
 Copyright (C) 2012-2013 Christian Dywan <christian@twotoasts.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
*/

namespace Sokoke {
    extern static bool message_dialog (Gtk.MessageType type, string short, string detailed, bool modal);
}

namespace Midori {
    namespace Download {
        public static bool is_finished (WebKit.Download download) {
#if !HAVE_WEBKIT2
            switch (download.status) {
                case WebKit.DownloadStatus.FINISHED:
                case WebKit.DownloadStatus.CANCELLED:
                case WebKit.DownloadStatus.ERROR:
                    return true;
                default:
                    return false;
            }
#else
            if (download.estimated_progress == 1)
                return true;
            return false;
#endif
        }

        public static int get_type (WebKit.Download download) {
            return download.get_data<int> ("midori-download-type");
        }

        public static void set_type (WebKit.Download download, int type) {
            download.set_data<int> ("midori-download-type", type);
        }

#if HAVE_WEBKIT2
        public static string get_filename (WebKit.Download download) {
            return download.get_data<string> ("midori-download-filename");
        }
        public static void set_filename (WebKit.Download download, string name) {
            stdout.printf("set_filename::filename[%s]\n", name);
            download.set_data<string> ("midori-download-filename", name);
        }
#endif
        public static double get_progress (WebKit.Download download) {
#if !HAVE_WEBKIT2
            /* Avoid a bug in WebKit */
            if (download.status == WebKit.DownloadStatus.CREATED)
                return 0.0;
            return download.progress;
#else
            return download.estimated_progress;
#endif
        }
        public static string get_website (WebKit.Download download) {
            string? website = download.request.uri;
            return "%s".printf (website);
        }

        public static string get_remaining (WebKit.Download download) {
            //WebKitURIResponse *response = webkit_download_get_response(download);
            //dialog->contentLength = webkit_uri_response_get_content_length(response);
            ///WebKit.WebContext m_webContext = WebKit.WebContext.get_default();
            WebKit.URIResponse response = download.get_response();
            uint64 total_size = response.get_content_length(), current_size = download.get_received_data_length ();
            double elapsed = download.elapsed_time,
               diff = elapsed / current_size;
	if(diff < 0.000001) diff += 0.000001; 
            double estimated = (total_size - current_size) * diff;
            int hour = 3600, minute = 60;
            int hours = (int)(estimated / hour),
                minutes = (int)((estimated - (hours * hour)) / minute),
                seconds = (int)((estimated - (hours * hour) - (minutes * minute)));
            string hours_ = ngettext ("%d hour", "%d hours", hours).printf (hours);
            string minutes_ = ngettext ("%d minute", "%d minutes", minutes).printf (minutes);
            string seconds_ = ngettext ("%d second", "%d seconds", seconds).printf (seconds);

            string eta = "";
            if (estimated > 0) {
                if (hours > 0)
                    eta = hours_ + ", " + minutes_;
                else if (minutes >= 10)
                    eta = minutes_;
                else if (minutes < 10 && minutes > 0)
                    eta = minutes_ + ", " + seconds_;
                else if (seconds > 0)
                    eta = seconds_;
                if (eta != "")
            /* i18n: Download tooltip (estimated time) : - 1 hour, 5 minutes remaning */
                    eta = _("%s").printf (eta);
            }
            return "%s".printf (eta);
        }
        
        public static string get_size (WebKit.Download download) {
            string size = "%s".printf (format_size (download.get_received_data_length ()));
            return "%s".printf (size);
        }

        public static string get_tooltip (WebKit.Download download) {
#if !HAVE_WEBKIT2
            string filename = Midori.Download.get_basename_for_display (download.destination_uri);
            /* i18n: Download tooltip (size): 4KB of 43MB */
            string size = _("%s of %s").printf (
                format_size (download.current_size),
                format_size (download.total_size));

            /* Finished, no speed or remaining time */
            if (is_finished (download) || download.status == WebKit.DownloadStatus.CREATED)
                return "%s\n%s".printf (filename, size);

            uint64 total_size = download.total_size, current_size = download.current_size;
            double elapsed = download.get_elapsed_time (),
               diff = elapsed / current_size,
               estimated = (total_size - current_size) * diff;
            int hour = 3600, minute = 60;
            int hours = (int)(estimated / hour),
                minutes = (int)((estimated - (hours * hour)) / minute),
                seconds = (int)((estimated - (hours * hour) - (minutes * minute)));
            string hours_ = ngettext ("%d hour", "%d hours", hours).printf (hours);
            string minutes_ = ngettext ("%d minute", "%d minutes", minutes).printf (minutes);
            string seconds_ = ngettext ("%d second", "%d seconds", seconds).printf (seconds);
            double last_time = download.get_data<int> ("last-time");

            string eta = "";
            if (estimated > 0) {
                if (hours > 0)
                    eta = hours_ + ", " + minutes_;
                else if (minutes >= 10)
                    eta = minutes_;
                else if (minutes < 10 && minutes > 0)
                    eta = minutes_ + ", " + seconds_;
                else if (seconds > 0)
                    eta = seconds_;
                if (eta != "")
            /* i18n: Download tooltip (estimated time) : - 1 hour, 5 minutes remaning */
                    eta = _(" - %s remaining").printf (eta);
            }

            string speed = "";
            uint64? last_size = download.get_data<uint64?> ("last-size");
            if (last_size != null && elapsed != last_time) {
                speed = format_size ((uint64)(
                    (current_size - last_size) / (elapsed - last_time)));
            }
            else
                /* i18n: Unknown number of bytes, used for transfer rate like ?B/s */
                speed = _("?B");
            /* i18n: Download tooltip (transfer rate): (130KB/s) */
            speed = _(" (%s/s)").printf (speed);

            if (elapsed - last_time > 0.0) {
                download.set_data<int> ("last-time", (int)elapsed);
                download.set_data<uint64?> ("last-size", current_size);
            }

            return "%s\n%s %s%s".printf (filename, size, speed, eta);
#else
            string filename = Midori.Download.get_basename_for_display (download.destination);

            //string size = "%s".printf (format_size (download.get_received_data_length ()));
            //string speed = "";
            //speed = format_size ((uint64)((download.get_received_data_length () * 1.0) / download.elapsed_time));
            //speed = _(" (%s/s)").printf (speed);
            //string progress = "%d%%".printf( (int) (download.get_estimated_progress ()*100));
            //if (is_finished (download))
                //return "%s\n %s".printf (filename, size);
            //return "%s\n %s - %s".printf (filename, speed, progress);
            return "%s".printf (filename);
#endif
        }

        public static string get_content_type (WebKit.Download download, string? mime_type) {
#if HAVE_WEBKIT2
            string? content_type = ContentType.guess (download.response.suggested_filename == null ?
                          download.destination : download.response.suggested_filename,
                          null, null);
#else
            string? content_type = ContentType.guess (download.suggested_filename, null, null);
#endif
            if (content_type == null) {
                content_type = ContentType.from_mime_type (mime_type);
                if (content_type == null)
                    content_type = ContentType.from_mime_type ("application/octet-stream");
            }
            return content_type;
        }

        public static bool has_wrong_checksum (WebKit.Download download) {
            int status = download.get_data<int> ("checksum-status");
            if (status == 0) {
                /* Link Fingerprint */
                #if HAVE_WEBKIT2
                string? original_uri = download.request.uri;
                #else
                string? original_uri = download.network_request.get_data<string> ("midori-original-uri");
                if (original_uri == null)
                    original_uri = download.get_uri ();
                #endif
                string? fingerprint;
                ChecksumType checksum_type = URI.get_fingerprint (original_uri, out fingerprint, null);
                /* By default, no wrong checksum */
                status = 2;
                if (fingerprint != null) {
                    try {
                        #if HAVE_WEBKIT2
                        string filename = Filename.from_uri (download.destination);
                        #else
                        string filename = Filename.from_uri (download.destination_uri);
                        #endif
                        string contents;
                        size_t length;
                        bool y = FileUtils.get_contents (filename, out contents, out length);
                        string checksum = Checksum.compute_for_string (checksum_type, contents, length);
                        /* Checksums are case-insensitive */
                        if (!y || fingerprint.ascii_casecmp (checksum) != 0)
                            status = 1; /* wrong checksum */
                    }
                    catch (Error error) {
                        status = 1; /* wrong checksum */
                    }
                }
                download.set_data<int> ("checksum-status", status);
            }
            return status == 1;

        }

        public static bool re_download (WebKit.Download download) { //throws Error {
            /*
            WebKitWebContext* m_webContext = webkit_web_context_get_default();
            const gchar *downloadUri = "http://127.0.0.1:8000/uex_15.0.0.9_i386.deb";
            WebKitDownload* download = webkit_web_context_download_uri(m_webContext, downloadUri);
            //webkit_download_set_destination(download, g_filename_to_uri(destination, 0, 0));
            WebKitURIRequest* request = webkit_download_get_request(download);
            */
            WebKit.WebContext m_webContext = WebKit.WebContext.get_default();
            string? original_uri = download.request.uri;
            WebKit.Download downloadt = m_webContext.download_uri(original_uri);
            downloadt.get_request();
            return true;
        }

        void new_download_cancel_cb (Gtk.Widget widget, Gtk.Widget dialog) {
            //gtk_widget_get_parent_window (widget)
            //widget.get_parent_window().destroy();
            //widget.destroy();
            dialog.destroy();
        }

        public static bool  new_download () { //throws Error {
            stdout.printf("new _download\n");

            var dialog = new Gtk.Dialog.with_buttons ("手动下载",
                null,
                Gtk.DialogFlags.DESTROY_WITH_PARENT, //|Gtk.DialogFlags.MODAL,
                null, null);

            dialog.set_default_size(600, 150);

            //dialog.set_icon_name (Gtk.STOCK_PROPERTIES);
//            dialog.set_response_sensitive (Gtk.ResponseType.HELP, false);

            var hbox1 = new Gtk.HBox (false, 0);
            var hbox3 = new Gtk.HBox (false, 0);
            var hbox4 = new Gtk.HBox (false, 0);
            var vbox = new Gtk.VBox (false, 0);
            hbox1.set_spacing(5);
            hbox3.set_spacing(5);
            hbox4.set_spacing(5);
            vbox.set_spacing(10);
            (dialog.get_content_area () as Gtk.Box).pack_start (vbox, false, false, 0);
            dialog.get_content_area ().set_spacing(4);
            dialog.get_content_area ().set_border_width(20);
            
            //this.description_label.set_markup (this.description);
            //this.description_label.set_line_wrap (true);
            //vbox.pack_start (this.description_label, false, false, 4);

            var label1 = new Gtk.Label("网址:                   ");
            var entry1 = new Gtk.Entry ();
            hbox1.pack_start (label1, false, false, 0);
            hbox1.pack_start (entry1, true, true, 0);

            var label3 = new Gtk.Label("文件名:               ");
            var entry3 = new Gtk.Entry ();
            hbox3.pack_start (label3, false, false, 0);
            hbox3.pack_start (entry3, true, true, 0);

            var dbtn = new Gtk.Button.with_label("下载");
            var cbtn = new Gtk.Button.with_label("取消");
            hbox4.pack_start (dbtn, false, false, 0);
            hbox4.pack_start (cbtn, false, false, 0);

            cbtn.clicked.connect (() => {
                dialog.destroy();
            });

            entry1.changed.connect (() => {
                entry3.set_text(File.new_for_uri (entry1.get_text()).get_basename());
            });

            dbtn.clicked.connect (() => {
                WebKit.WebContext m_webContext = WebKit.WebContext.get_default();
                string? uri = entry1.get_text();
                WebKit.Download downloadt = m_webContext.download_uri(uri);
                set_filename(downloadt, entry3.get_text());
                downloadt.get_request();
            });

            
            //g_signal_connect(G_OBJECT(cbtn), "clicked", G_CALLBACK(_action_download_cancel_activate), dialog);
            //cbtn.clicked.connect(new_download_cancel_cb);
            /*

            string filename = File.new_for_uri (uri).get_basename ();
            GFile *destFile = g_file_new_for_uri(gtk_entry_get_text(GTK_ENTRY(btn)));
            gchar *filename = g_file_get_basename(destFile);
            gtk_entry_set_text(GTK_ENTRY(dialog), filename);
            g_free(filename);
            g_object_unref(destFile);

            
            g_signal_connect (G_OBJECT (entry1), "changed", G_CALLBACK (_action_download_filename_change_activate), entry3);
            GtkWidget *dbtn = gtk_button_new_with_label("下载");
            //g_signal_connect(G_OBJECT(dialog->dbtn), "clicked", G_CALLBACK(downloadBtnCallback), dialog);
            
            cbtn.clicked.connect ((cbtn, dialog) => {
                dialog.destroy();
            )};
            */
            
            vbox.pack_start (hbox1, false, false, 0);
            vbox.pack_start (hbox3, false, false, 0);
            vbox.pack_start (hbox4, false, false, 0);
            dialog.get_content_area ().show_all ();
            dialog.show ();
            /*
            GtkWidget* dialog;

            dialog = gtk_dialog_new_with_buttons ("Download Manager Dialog", GTK_WINDOW (browser),
                    GTK_DIALOG_DESTROY_WITH_PARENT |GTK_DIALOG_MODAL |GTK_DIALOG_NO_SEPARATOR, NULL, NULL);
            GtkBox *contentArea = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));
            gtk_box_set_spacing(contentArea, 4);

            gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 150);
            gtk_window_set_title(GTK_WINDOW(dialog), "下载文件");
            gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), FALSE);
            gtk_container_set_border_width(GTK_CONTAINER(dialog), 20);

            GtkWidget *h1box,*h3box,*h4box,*vbox;
            GtkWidget *cbtn;
            GtkWidget *label1, *label3;

            //void gtk_box_set_spacing (GtkBox *box, gint    spacing)
            h1box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            h3box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            h4box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
            vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

            label1 = gtk_label_new("网址:                  ");
            GtkWidget *entry1 = gtk_entry_new();

            gtk_box_pack_start(GTK_BOX(h1box), label1, FALSE, FALSE, 0);  
            gtk_box_pack_start(GTK_BOX(h1box), entry1, TRUE, TRUE, 0); 

            label3 = gtk_label_new("文件名:               ");
            GtkWidget *entry3 = gtk_entry_new();

            gtk_box_pack_start(GTK_BOX(h3box), label3, FALSE, FALSE, 0);  
            gtk_box_pack_start(GTK_BOX(h3box), entry3, TRUE, TRUE, 0);

            g_signal_connect (G_OBJECT (entry1), "changed", G_CALLBACK (_action_download_filename_change_activate), entry3);
            GtkWidget *dbtn = gtk_button_new_with_label("下载");
            //g_signal_connect(G_OBJECT(dialog->dbtn), "clicked", G_CALLBACK(downloadBtnCallback), dialog);

            cbtn = gtk_button_new_with_label("取消");
            g_signal_connect(G_OBJECT(cbtn), "clicked", G_CALLBACK(_action_download_cancel_activate), dialog);

            gtk_box_pack_start(GTK_BOX(h4box), dbtn, FALSE, FALSE, 0);  
            gtk_box_pack_start(GTK_BOX(h4box), cbtn, FALSE, FALSE, 0);

            gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(h1box),FALSE,FALSE,0);  
            gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(h3box),FALSE,FALSE,0); 
            gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(h4box),FALSE,FALSE,0);

            gtk_container_add(GTK_CONTAINER(contentArea),vbox);
            gtk_widget_show_all(GTK_WIDGET(contentArea));  

            midori_dialog_run (GTK_DIALOG (dialog));        
            */

            stdout.printf("new_download end\n");
            return true;
        }

        public static bool action_clear (WebKit.Download download, Gtk.Widget widget) throws Error {
#if !HAVE_WEBKIT2
            switch (download.status) {
                case WebKit.DownloadStatus.CREATED:
                case WebKit.DownloadStatus.STARTED:
                    download.cancel ();
                    break;
                case WebKit.DownloadStatus.FINISHED:
                    if (open (download, widget))
                        return true;
                    break;
                case WebKit.DownloadStatus.CANCELLED:
                    return true;
                default:
                    critical ("action_clear: %d", download.status);
                    warn_if_reached ();
                    break;
            }
            #else

            if (download.estimated_progress < 1) {
                download.cancel ();
            } else {
                if (open (download, widget))
                    return true;
            }
#endif
            return false;
        }

        public static string action_stock_id (WebKit.Download download) {
#if !HAVE_WEBKIT2
            switch (download.status) {
                case WebKit.DownloadStatus.CREATED:
                case WebKit.DownloadStatus.STARTED:
                    return Gtk.Stock.CANCEL;
                case WebKit.DownloadStatus.FINISHED:
                    if (has_wrong_checksum (download))
                        return Gtk.Stock.DIALOG_WARNING;
                    return Gtk.Stock.OPEN;
                case WebKit.DownloadStatus.CANCELLED:
                    return Gtk.Stock.CLEAR;
                case WebKit.DownloadStatus.ERROR:
                    return Gtk.Stock.DIALOG_ERROR;
                default:
                    critical ("action_stock_id: %d", download.status);
                    warn_if_reached ();
                    return Gtk.Stock.MISSING_IMAGE;
            }
#else
            if (download.estimated_progress == 1)
                if (has_wrong_checksum (download))
                    return Gtk.Stock.DIALOG_WARNING;
                else
                    return Gtk.Stock.OPEN;
            return Gtk.Stock.CANCEL;
#endif
        }

        /* returns whether an application was successfully launched to handle the file */
        public static bool open (WebKit.Download download, Gtk.Widget widget) throws Error {
            if (has_wrong_checksum (download)) {
                Sokoke.message_dialog (Gtk.MessageType.WARNING,
                     _("The downloaded file is erroneous."),
                     _("The checksum provided with the link did not match. This means the file is probably incomplete or was modified afterwards."),
                     true);
                return true;
            } else {
                //var browser = widget.get_toplevel ();//.get_transient_for();
                var browser = (widget.get_toplevel () as Gtk.Window).get_transient_for ();
                Tab? tab = null;
                browser.get ("tab", &tab);
                if (tab != null)
                #if HAVE_WEBKIT2
                    return tab.open_uri (download.destination);
                #else
                    return tab.open_uri (download.destination_uri);
                #endif
            }
            return false;
        }

        public unowned string fallback_extension (string? extension, string mime_type) {
            if (extension != null && extension[0] != '\0')
                return extension;
            if ("css" in mime_type)
                return ".css";
            if ("javascript" in mime_type)
                return ".js";
            if ("html" in mime_type)
                return ".htm";
            if ("plain" in mime_type)
                return ".txt";
            return "";
        }

        public string clean_filename (string filename) {
            #if HAVE_WIN32
            return filename.delimit ("/\\<>:\"|?* ", '_');
            #else
            return filename.delimit ("/ ", '_');
            #endif
        }

        public string get_suggested_filename (WebKit.Download download) {
#if !HAVE_WEBKIT2
            /* https://bugs.webkit.org/show_bug.cgi?id=83161
               https://d19vezwu8eufl6.cloudfront.net/nlp/slides%2F03-01-FormalizingNB.pdf */
            return clean_filename (download.get_suggested_filename ());
#else
            string name = get_filename (download);
            if (name == null)
                return "";
            return name;
#endif
        }

        /**
         * Returns a filename of the form "name.ext" to use as a suggested name for
         * a download of the given uri
         */
        public string get_filename_suggestion_for_uri (string mime_type, string uri) {
            return_val_if_fail (Midori.URI.is_location (uri), uri);
            string filename = File.new_for_uri (uri).get_basename ();
            if (uri.index_of_char ('.') == -1)
                return Path.build_filename (filename, fallback_extension (null, mime_type));
            return filename;
        }

        public static string? get_extension_for_uri (string uri, out string basename = null) {
            basename = null;
            /* Find the last slash and the last period *after* the last slash. */
            int last_slash = uri.last_index_of_char ('/');
            /* Huh, URI without slashes? */
            if (last_slash == -1)
                return null;
            int period = uri.last_index_of_char ('.', last_slash);
            if (period == -1)
                return null;
            /* Exclude the query: ?query=foobar */
            int query = uri.last_index_of_char ('?', period);
            /* The extension, or "." if it ended with a period */
            string extension = uri.substring (period, query - period);
            basename = uri.substring (0, period);
            return extension;

        }

        public string get_unique_filename (string filename) {
            if (Posix.access (filename, Posix.F_OK) == 0) {
                string basename;
                string? extension = get_extension_for_uri (filename, out basename);
                string? new_filename = null;
                int i = 0;
                do {
                    new_filename = "%s-%d%s".printf (basename, i++, extension ?? "");
                } while (Posix.access (new_filename, Posix.F_OK) == 0);
                return new_filename;
            }
            return filename;
        }

        /**
         * Returns a string showing a file:// URI's intended filename on
         * disk, suited for displaying to a user.
         * 
         * The string returned is the basename (final path segment) of the
         * filename of the uri. If the uri is invalid, not file://, or has no
         * basename, the uri itself is returned.
         * 
         * Since: 0.5.7
         **/
        public static string get_basename_for_display (string uri) {
            try {
                string filename = Filename.from_uri (uri);
                if(filename != null && filename != "")
                    return Path.get_basename (filename);
            } catch (Error error) { }
            return uri;
        }

        public string prepare_destination_uri (WebKit.Download download, string? folder) {
            string suggested_filename = get_suggested_filename (download);
            string basename = Path.get_basename (suggested_filename);
            string download_dir;
            if (folder == null) {
                download_dir = Paths.get_tmp_dir ();
                Katze.mkdir_with_parents (download_dir, 0700);
            }
            else
                download_dir = folder;
            string destination_filename = Path.build_filename (download_dir, basename);
            try {
                return Filename.to_uri (get_unique_filename (destination_filename));
            }
            catch (Error error) {
                return "file://" + destination_filename;
            }
        }

        /**
         * Returns whether it seems possible to save @download to the path specified by
         * @destination_uri, considering space on disk and permissions
         */
        public static bool has_enough_space (WebKit.Download download, string destination_uri, bool quiet=false) {
#if !HAVE_WEBKIT2
            var folder = File.new_for_uri (destination_uri).get_parent ();
            bool can_write;
            uint64 free_space;
            try {
                var info = folder.query_filesystem_info ("filesystem::free");
                free_space = info.get_attribute_uint64 ("filesystem::free");
                info = folder.query_info ("access::can-write", 0);
                can_write = info.get_attribute_boolean ("access::can-write");
            }
            catch (Error error) {
                can_write = false;
                free_space = 0;
            }

            if (free_space < download.total_size || !can_write) {
                string message;
                string detailed_message;
                if (!can_write) {
                    message = _("The file \"%s\" can't be saved in this folder.").printf (
                        Midori.Download.get_basename_for_display (destination_uri));
                    detailed_message = _("You don't have permission to write in this location.");
                }
                else if (free_space < download.total_size) {
                    message = _("There is not enough free space to download \"%s\".").printf (
                        Midori.Download.get_basename_for_display (destination_uri));
                    detailed_message = _("The file needs %s but only %s are left.").printf (
                        format_size (download.total_size), format_size (free_space));
                }
                else
                    assert_not_reached ();
                if (!quiet)
                    Sokoke.message_dialog (Gtk.MessageType.ERROR, message, detailed_message, false);
                return false;
            }
#endif
            return true;
        }
    }
}
