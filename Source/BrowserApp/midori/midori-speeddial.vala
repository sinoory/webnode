/*
 Copyright (C) 2011-2013 Christian Dywan <christian@twotoats.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 See the file COPYING for the full license text.
 
 Modify by ZhangRuili
 2014.12.02 save_message() 新建快捷方式时，增加title字段，默认与uri相同
 2014.12.02 实现load_changed(), get_thumb(), save_thumbnail()函数，支持显示缩略图
*/

namespace Katze {
    extern static string mkdir_with_parents (string pathname, int mode);
}

namespace Sokoke {
    extern static string js_script_eval (void* ctx, string script, void* error);
}

namespace Midori {
    public errordomain SpeedDialError {
        INVALID_MESSAGE,
        NO_ACTION,
        NO_ID,
        NO_URL,
        NO_TITLE,
        NO_ID2,
        INVALID_ACTION,
    }

    public class SpeedDial : GLib.Object {
        string filename;
        string? html = null;
        List<Spec> thumb_queue = null;
        WebKit.WebView thumb_view = null;
        Spec? spec = null;
	//added by wangyl 2015.8.7 start
        public string imagename{get; set; default = null;}
        public string imagetitle{get; set; default = null;}
        public string imageid{get; set; default = null;}
        bool Load_status = false;//Whether to download
        public int    load_time = 30;//Maximum download time
        public uint  timer_id;
        public GLib.KeyFile keyfile;
        public bool close_buttons_left { get; set; default = false; }
        public signal void refresh ();
        public signal void refresh1 ();
	//added by wangyl 2015.8.7 end
        public class Spec {
            public string dial_id;
            public string uri;
            public Spec (string dial_id, string uri) {
                this.dial_id = dial_id;
                this.uri = uri;
            }
        }

        public SpeedDial (string new_filename, string? fallback = null) {
            filename = new_filename;
            thumb_queue = new GLib.List<Spec> ();
            keyfile = new GLib.KeyFile ();
            try {
                keyfile.load_from_file (filename, GLib.KeyFileFlags.NONE);
	        foreach (string tile in keyfile.get_groups ()) {
                 try {
		      string load_status1 = keyfile.get_string (tile, "load_status");
		      if(load_status1 == "0" ||load_status1 == "1" ) keyfile.set_string (tile, "load_status", "2");
                 }
                    catch (GLib.Error img_error) {
                        /* img and uri can be missing */
                    }
	       }
            }
            catch (GLib.Error io_error) {
                string json;
                size_t len;
                try {
                    FileUtils.get_contents (fallback ?? (filename + ".json"),
                                            out json, out len);
                }
                catch (GLib.Error fallback_error) {
                    json = "'{}'";
                    len = 5;
                }

                var script = new StringBuilder.sized (len);
                script.append ("var json = JSON.parse (");
                script.append_len (json, (ssize_t)len);
                script.append ("""
        );
        var keyfile = '';
        for (var i in json['shortcuts']) {
        var tile = json['shortcuts'][i];
        keyfile += '[Dial ' + tile['id'].substring (1) + ']\n'
                +  'imageid=' + tile['imageid'] + '\n'
                +  'load_status=' + tile['load_status'] + '\n'
                +  'uri=' + tile['href'] + '\n'
                +  'img=' + tile['img'] + '\n'
                +  'title=' + tile['title'] + '\n\n';
        }
        var columns = json['width'] ? json['width'] : 5;
        var rows = json['shortcuts'] ? json['shortcuts'].length / columns : 0;
        keyfile += '[settings]\n'
                +  'columns=' + columns + '\n'
                +  'rows=' + (rows > 5 ? rows : 5) + '\n\n';
        keyfile;
                    """);

                try {
                    keyfile.load_from_data (
                        Sokoke.js_script_eval (null, script.str, null),
                        -1, 0);
                }
                catch (GLib.Error eval_error) {
                    GLib.critical ("Failed to parse %s as speed dial JSON: %s",
                                   fallback ?? (filename + ".json"), eval_error.message);
                }
                Katze.mkdir_with_parents (
                    Path.build_path (Path.DIR_SEPARATOR_S,
                                     Environment.get_user_cache_dir (),
                                     PACKAGE_NAME, "thumbnails"), 0700);
                foreach (string tile in keyfile.get_groups ()) {
                    try {
		         string load_status1 = keyfile.get_string (tile, "load_status");
		         if(load_status1 == "0" ||load_status1 == "1" ) keyfile.set_string (tile, "load_status", "2");
                         string img = keyfile.get_string (tile, "img");
                         keyfile.remove_key (tile, "img");
                         string uri = keyfile.get_string (tile, "uri");
                         if (img != null && uri[0] != '\0' && uri[0] != '#') {
                            uchar[] decoded = Base64.decode (img);
                            FileUtils.set_data (build_thumbnail_path (uri), decoded);
                        }
                    }
                    catch (GLib.Error img_error) {
                        /* img and uri can be missing */
                    }
                }
            }
        }
       #if 0
          public unowned string get_html () throws Error {
            bool load_missing = true;

            if (html != null)
                return html;

            string? head = null;
            string filename = Paths.get_res_filename ("speeddial-head.html");
            if (keyfile != null
             && FileUtils.get_contents (filename, out head, null)) {
                string header = head.replace ("{title}", _("Speed Dial")).
                    replace ("{click_to_add}", _("Click to add a shortcut")).
                    replace ("{enter_shortcut_address}", _("Enter shortcut address")).
                    replace ("{enter_shortcut_name}", _("Enter shortcut title")).
                    replace ("{are_you_sure}", _("Are you sure you want to delete this shortcut?"));
                var markup = new StringBuilder (header);

                uint slot_count = 1;
                string dial_id = get_next_free_slot (out slot_count);
                uint next_slot = dial_id.substring (5, -1).to_int ();

                /* Try to guess the best X by X grid size */
                uint grid_index = 3;
                while ((grid_index * grid_index) < slot_count)
                    grid_index++;

                /* Percent width size of one slot */
                uint slot_size = (100 / grid_index);

                /* No editing in private/ app mode or without scripts */
                markup.append_printf (
                    "%s<style>.cross { display:none }</style>%s" +
                    "<style> div.shortcut { height: %d%%; width: %d%%; }</style>\n",
                    Paths.is_readonly () ? "" : "<noscript>",
                    Paths.is_readonly () ? "" : "</noscript>",
                    slot_size + 1, slot_size - 4);

                /* Combined width of slots should always be less than 100%.
                 * Use half of the remaining percentage as a margin size */
                uint div_factor;
                if (slot_size * grid_index >= 100 && grid_index > 4)
                    div_factor = 8;
                else
                    div_factor = 2;
                uint margin = (100 - ((slot_size - 4) * grid_index)) / div_factor;
                if (margin > 9)
                    margin = margin % 10;

                markup.append_printf (
                    //"<style> body { overflow:hidden } #content { margin-left: %u%%; }</style>", margin);//modified by wangyl 2015.6.24
		    "<style> body { width:60%; height:75%;overflow:hidden } #content { margin-left: %u%%;margin-top: %u%%; }</style>",40,12);
                if (close_buttons_left)
                    markup.append_printf (
                        "<style>.cross { left: -14px }</style>");

                foreach (string tile in keyfile.get_groups ()) {
                    try {
                        string uri = keyfile.get_string (tile, "uri");
                        if (uri != null && uri.str ("://") != null && tile.has_prefix ("Dial ")) {
                            string title = keyfile.get_string (tile, "title");
                            if (title == null) {
                                title = uri;
                            }
                            string thumb_filename = build_thumbnail_path (uri);
                            uint slot = tile.substring (5, -1).to_int ();
                            string encoded;
                            try {
                                uint8[] thumb;
                                FileUtils.get_data (thumb_filename, out thumb);
                                encoded = Base64.encode (thumb);
                            }
                            catch (FileError error) {
                                encoded = null;
                                if (load_missing)
                                    get_thumb (tile, uri);
                            }
                            markup.append_printf ("""
                                <div class="shortcut" id="%u"><div class="preview">
                                <a class="cross" href="#"></a>
                                <a href="%s"><img src="data:image/png;base64,%s" title='%s'></a>
                                </div><div class="title">%s</div></div>
                                """,
                                slot, uri, encoded ?? "", title, title ?? "");
                        }
                        else if (tile != "settings")
                            keyfile.remove_group (tile);
                    }
                    catch (KeyFileError error) { }
                }
		if(slot_count<9){
                   markup.append_printf ("""
                    <div class="shortcut" id="%u"><div class="preview new">
                    <a class="add" href="#"></a>
                    </div><div class="title">%s</div></div>
                    """,
                    next_slot,  _("Click to add a shortcut"));
		}
		else {
		   markup.append_printf (""" <div class="preview new"></div></div>""");
                }
                markup.append_printf ("</div>\n</body>\n</html>\n");
                html = markup.str;
            }
            else
                html = "";

            return html;
        }
	#endif
	
        public string get_next_free_slot (out uint count = null) {
            uint slot_count = 0;
            foreach (string tile in keyfile.get_groups ()) {
                try {
                    if (keyfile.has_key (tile, "uri"))
                        slot_count++;
                }
                catch (KeyFileError error) { }
            }
            count = slot_count;

            uint slot = 1;
            while (slot <= slot_count) {
                string tile = "Dial %u".printf (slot);
                if (!keyfile.has_group (tile))
                    return tile;
                slot++;
            }

            return "Dial %u".printf (slot_count + 1);
        }

        public void add (string uri, string title, Gdk.Pixbuf? img) {
                foreach (string tile in keyfile.get_groups ()){
                    try {
                               string image_id = keyfile.get_string (tile, "imageid");
                               if( title == image_id)return;
                   }
                   catch (KeyFileError error) { }
              }	 
             string id = get_next_free_slot ();
             keyfile.set_string (id, "imageid", title);
	    if(Load_status == false && load_time < 30)keyfile.set_string (id, "load_status", "0");
	    else keyfile.set_string (id, "load_status", "1");//0 for waiting for loading , 1 for  start loading ,2 for finish loading  
            uint slot = id.substring (5, -1).to_int ();
            try {
                save_message ("speed_dial-save-add %u %s %s".printf (slot, uri,title));
            }
            catch (Error error) {
                critical ("Failed to add speed dial thumbnail: %s", error.message);
            }
        }

        public void add_with_id (string id, string uri, string title, Gdk.Pixbuf? img) {
            keyfile.set_string (id, "uri", uri);
            keyfile.set_string (id, "title", title);
	    keyfile.set_string (id, "load_status", "2");
	  
            Katze.mkdir_with_parents (Path.build_path (Path.DIR_SEPARATOR_S,
                Paths.get_cache_dir (), "thumbnails"), 0700);
            string filename = build_thumbnail_path (uri);
	    imagename = filename;			
            imagetitle = title;
            imageid  = keyfile.get_string (id, "imageid");
            try {
                img.save (filename, "png", null, "compression", "7", null);
            }
            catch (Error error) {
                critical ("Failed to save speed dial thumbnail: %s", error.message);
            }
            save ();
            refresh ();
       }
			
        string build_thumbnail_path (string filename) {
            string thumbnail = Checksum.compute_for_string (ChecksumType.MD5, filename) + ".png";
            return Path.build_filename (Paths.get_cache_dir (), "thumbnails", thumbnail);
        }

      
        public void save_message (string message) throws Error {
            if (!message.has_prefix ("speed_dial-save-"))
                throw new SpeedDialError.INVALID_MESSAGE ("Invalid message '%s'", message);

            string msg = message.substring (16, -1);
            string[] parts = msg.split (" ", 4);
            if (parts[0] == null)
                throw new SpeedDialError.NO_ACTION ("No action.");
            string action = parts[0];
            if (parts[1] == null)
                throw new SpeedDialError.NO_ID ("No ID argument.");
            string dial_id = "Dial " + parts[1];

                if (action == "delete") {
                 int n = 0;
		int n_th = 0;
		string Load_status1=null;
		string Load_status2=null;
                   foreach (string tile in keyfile.get_groups ()){
		   Load_status1 =  keyfile.get_string (tile, "load_status");
		     if(Load_status1 == "0" ||Load_status1 == "1" ) n_th++;
                          try {
                             string image_id = keyfile.get_string (tile, "imageid");					
                               if(parts[1] == image_id){
                                        string uri = keyfile.get_string (tile, "uri");
				  
				    if(Load_status1 == "0" ||Load_status1 == "1" ){
  				        keyfile.remove_group (tile);
 				       break;
				    }	  
                                        foreach (string tile1 in keyfile.get_groups ()){
                                                try {
                                                            if(uri == keyfile.get_string (tile1, "uri"))n++;
                                                 }
                                               catch (KeyFileError error) { }
                                       }
										
                                       string file_path = build_thumbnail_path (uri);
                                       keyfile.remove_group (tile);
                                        if(n==1)FileUtils.unlink (file_path);
                                        break;
                              }
                         }		
                        catch (KeyFileError error) { }
                      }
		 if (Load_status1 == "0" ){
		 	Spec? spec1 = thumb_queue.nth_data (n_th-1);
			 thumb_queue.remove (spec1);
			 return;
                 }
		 if(Load_status1 == "1"){
		   if (thumb_view == null)return;
		   if(load_time<30){
                      Source.remove(timer_id);
		      Load_status = false;
                      load_time = 30;
                   }
		   thumb_view.load_failed.disconnect (load_failed);
                   thumb_view.load_changed.disconnect (load_changed);
		   var offscreen = (thumb_view.parent as Gtk.OffscreenWindow);
		   thumb_queue.remove (spec);
                   offscreen.remove(thumb_view);
                   thumb_view.destroy();
                   thumb_view = new WebKit.WebView ();
                   thumb_view.get_settings().set (
                     "enable-javascript", true,
                     "enable-plugins", false,
                     "auto-load-images", true,
                     "enable-html5-database", false,
                     "enable-html5-local-storage", false,
                     "enable-java", false);
                  offscreen.add (thumb_view);
                  thumb_view.set_size_request (800, 600);
                  offscreen.show_all ();	
                  if (thumb_queue.length () > 0) {
	            spec = thumb_queue.nth_data (0);
	            foreach (string tile in keyfile.get_groups ()){
		      Load_status2 =  keyfile.get_string (tile, "load_status");
		       if(Load_status2 == "0"){
                         keyfile.set_string (tile, "load_status", "1");
                         break;
                      }
	            }
	           thumb_view.load_changed.connect (load_changed);
	           thumb_view.load_failed.connect (load_failed);
                   timer_id =  GLib.Timeout.add(1000,check_load_status);
	           thumb_view.load_uri (spec.uri);
                 }
	         return;
              }
              save();
              refresh1 ();
             }
                else if (action == "add") {
                    if (parts[2] == null)
                        throw new SpeedDialError.NO_URL ("No URL argument.");
                    keyfile.set_string (dial_id, "uri", parts[2]);
                    keyfile.set_string (dial_id, "title", parts[2]); //ZRL create default title.
                    get_thumb (dial_id, parts[2]);
                }
                else if (action == "rename") {
                    if (parts[2] == null)
                        throw new SpeedDialError.NO_TITLE ("No title argument.");
                    string title = parts[2];
                    keyfile.set_string (dial_id, "title", title);
                }
                else if (action == "swap") {
                    if (parts[2] == null)
                        throw new SpeedDialError.NO_ID2 ("No ID2 argument.");
                    string dial2_id = "Dial " + parts[2];

                    string uri = keyfile.get_string (dial_id, "uri");
                    string title = keyfile.get_string (dial_id, "title");
                    string uri2 = keyfile.get_string (dial2_id, "uri");
                    string title2 = keyfile.get_string (dial2_id, "title");

                    keyfile.set_string (dial_id, "uri", uri2);
                    keyfile.set_string (dial2_id, "uri", uri);
                    keyfile.set_string (dial_id, "title", title2);
                    keyfile.set_string (dial2_id, "title", title);
                }
                else
                    throw new SpeedDialError.INVALID_ACTION ("Invalid action '%s'", action);
        }

        void save () {
            html = null;

            try {
                FileUtils.set_contents (filename, keyfile.to_data ());
            }
            catch (Error error) {
                critical ("Failed to update speed dial: %s", error.message);
            }
           // refresh ();
        }

// ZRL implement for Webkit2gtk API
#if !HAVE_WEBKIT2
        void load_status (GLib.Object thumb_view_, ParamSpec pspec) {
            if (thumb_view.load_status != WebKit.LoadStatus.FINISHED
             && thumb_view.load_status != WebKit.LoadStatus.FAILED)
                return;
            thumb_view.notify["load-status"].disconnect (load_status);
            /* Schedule an idle to give the offscreen time to draw */
            Idle.add (save_thumbnail);
        }
#else
        void load_changed (GLib.Object thumb_view_, WebKit.LoadEvent load_event) {
            if (load_event != WebKit.LoadEvent.FINISHED)
                return;
             Load_status = true;
            thumb_view.load_changed.disconnect (load_changed);
            /* Schedule an idle to give the offscreen time to draw */
            Idle.add (save_thumbnail);
        }

        bool load_failed (GLib.Object thumb_view_, WebKit.LoadEvent load_event, string failingURI, void *error) {
            thumb_view.load_failed.disconnect (load_failed);
            thumb_view.load_changed.disconnect (load_changed);
             Load_status = true;
            Idle.add (save_thumbnail);
            return true;
        }
#endif

#if !HAVE_WEBKIT2
        bool save_thumbnail () {
            return_val_if_fail (spec != null, false);

            var offscreen = (thumb_view.parent as Gtk.OffscreenWindow);
            var pixbuf = offscreen.get_pixbuf ();
            int image_width = pixbuf.get_width (), image_height = pixbuf.get_height ();
            int thumb_width = 240, thumb_height = 160;
            float image_ratio = image_width / image_height;
            float thumb_ratio = thumb_width / thumb_height;
            int x_offset, y_offset, computed_width, computed_height;
            if (image_ratio > thumb_ratio) {
                computed_width = (int)(image_height * thumb_ratio);
                computed_height = image_height;
                x_offset = (image_width - computed_width) / 2;
                y_offset = 0;
            }
            else {
                computed_width = image_width;
                computed_height = (int)(image_width / thumb_ratio);
                x_offset = 0;
                y_offset = 0;
            }
            var sub = pixbuf;
            if (y_offset + computed_height <= image_height)
                sub = new Gdk.Pixbuf.subpixbuf (pixbuf, x_offset, y_offset, computed_width, computed_height);
            var scaled = sub.scale_simple (thumb_width, thumb_height, Gdk.InterpType.TILES);
            add_with_id (spec.dial_id, spec.uri, thumb_view.get_title () ?? spec.uri, scaled);

            thumb_queue.remove (spec);
            if (thumb_queue.length () > 0) {
                spec = thumb_queue.nth_data (0);
                thumb_view.notify["load-status"].connect (load_status);
                thumb_view.load_uri (spec.uri);
            }
            return false;
        }
#else
        bool save_thumbnail () {
            return_val_if_fail (spec != null, false);

            var offscreen = (thumb_view.parent as Gtk.OffscreenWindow);
            var pixbuf = offscreen.get_pixbuf ();
            int image_width = pixbuf.get_width (), image_height = pixbuf.get_height ();
            int thumb_width = 240, thumb_height = 160;
            float image_ratio = image_width / image_height;
            float thumb_ratio = thumb_width / thumb_height;
            int x_offset, y_offset, computed_width, computed_height;
            if (image_ratio > thumb_ratio) {
                computed_width = (int)(image_height * thumb_ratio);
                computed_height = image_height;
                x_offset = (image_width - computed_width) / 2;
                y_offset = 0;
            }
            else {
                computed_width = image_width;
                computed_height = (int)(image_width / thumb_ratio);
                x_offset = 0;
                y_offset = 0;
            }
            var sub = pixbuf;
            if (y_offset + computed_height <= image_height)
                sub = new Gdk.Pixbuf.subpixbuf (pixbuf, x_offset, y_offset, computed_width, computed_height);
            var scaled = sub.scale_simple (thumb_width, thumb_height, Gdk.InterpType.TILES);
            add_with_id (spec.dial_id, spec.uri, thumb_view.get_title () ?? spec.uri, scaled);

            thumb_queue.remove (spec);
	    if(load_time < 30) {
              Source.remove(timer_id);
	      Load_status = false;
              load_time = 30;
            }
            if (thumb_queue.length () > 0) {
                spec = thumb_queue.nth_data (0); 
	       foreach (string tile in keyfile.get_groups ()){
		  string Load_status2 =  keyfile.get_string (tile, "load_status");
		  if(Load_status2 == "0"){keyfile.set_string (tile, "load_status", "1");break;}
	     }
             thumb_view.load_changed.connect (load_changed);
             thumb_view.load_failed.connect (load_failed);
             timer_id =   GLib.Timeout.add(1000,check_load_status);
             thumb_view.load_uri (spec.uri);
            }
            return false;
        }
#endif
         bool  save_unfinished_thumbnail(){
              return_val_if_fail (spec != null, false);

              thumb_view.load_failed.disconnect (load_failed);
              thumb_view.load_changed.disconnect (load_changed);

              var offscreen = (thumb_view.parent as Gtk.OffscreenWindow);
              var pixbuf = offscreen.get_pixbuf ();
              int image_width = pixbuf.get_width (), image_height = pixbuf.get_height ();
              int thumb_width = 240, thumb_height = 160;
              float image_ratio = image_width / image_height;
              float thumb_ratio = thumb_width / thumb_height;
              int x_offset, y_offset, computed_width, computed_height;
              if (image_ratio > thumb_ratio) {
	           computed_width = (int)(image_height * thumb_ratio);
	           computed_height = image_height;
	           x_offset = (image_width - computed_width) / 2;
	          y_offset = 0;
              }
             else {
	          computed_width = image_width;
	          computed_height = (int)(image_width / thumb_ratio);
	           x_offset = 0;
	           y_offset = 0;
              }
             var sub = pixbuf;
             if (y_offset + computed_height <= image_height)
	      sub = new Gdk.Pixbuf.subpixbuf (pixbuf, x_offset, y_offset, computed_width, computed_height);
              var scaled = sub.scale_simple (thumb_width, thumb_height, Gdk.InterpType.TILES);
              add_with_id (spec.dial_id, spec.uri, thumb_view.get_title () ?? spec.uri, scaled);

               thumb_queue.remove (spec);
               offscreen.remove(thumb_view);
                thumb_view.destroy();
                thumb_view = new WebKit.WebView ();
               thumb_view.get_settings().set (
                   "enable-javascript", true,
                   "enable-plugins", false,
                   "auto-load-images", true,
                   "enable-html5-database", false,
                   "enable-html5-local-storage", false,
                   "enable-java", false);
              offscreen.add (thumb_view);
               thumb_view.set_size_request (800, 600);
               offscreen.show_all ();
	     if(load_time < 30) {
	        Source.remove(timer_id);
	        Load_status = false;
                 load_time = 30;
	     }
               if (thumb_queue.length () > 0) {
	     spec = thumb_queue.nth_data (0);
	       foreach (string tile in keyfile.get_groups ()){
		   string  Load_status2 =  keyfile.get_string (tile, "load_status");
		    if(Load_status2 == "0"){
			keyfile.set_string (tile, "load_status", "1");
			break;
		   }
	       	}
	     thumb_view.load_changed.connect (load_changed);
	     thumb_view.load_failed.connect (load_failed);
             timer_id =   GLib.Timeout.add(1000,check_load_status);
	      thumb_view.load_uri (spec.uri);
               }
               return false;		
          }
          bool check_load_status(){ 
	     if( Load_status == true){
                    Load_status = false;
                    load_time = 30;
                     return false;//delete timer
	     }
              else if( load_time == 0  && Load_status == false ){
                     Load_status = false;
                    load_time = 30;
		 save_unfinished_thumbnail();
                   return false;//delete timer
              }
            load_time--;
            return    true;//continue use timer
	}
        void get_thumb (string dial_id, string uri) {
#if !HAVE_WEBKIT2
            if (thumb_view == null) {
                thumb_view = new WebKit.WebView ();
                thumb_view.settings.set (
                    "enable-scripts", false,
                    "enable-plugins", false,
                    "auto-load-images", true,
                    "enable-html5-database", false,
                    "enable-html5-local-storage", false,
                    "enable-java-applet", false);
                var offscreen = new Gtk.OffscreenWindow ();
                offscreen.add (thumb_view);
                thumb_view.set_size_request (800, 600);
                offscreen.show_all ();
            }

            /* Don't load thumbnails already queued */
            foreach (var spec_ in thumb_queue)
                if (spec_.dial_id == dial_id)
                    return;

            thumb_queue.append (new Spec (dial_id, uri));
            if (thumb_queue.length () > 1)
                return;

            spec = thumb_queue.nth_data (0);
            thumb_view.notify["load-status"].connect (load_status);
            thumb_view.load_uri (spec.uri);
#else
            if (thumb_view == null) {
                thumb_view = new WebKit.WebView ();
                thumb_view.get_settings().set (
                    "enable-javascript", true,
                    "enable-plugins", false,
                    "auto-load-images", true,
                    "enable-html5-database", false,
                    "enable-html5-local-storage", false,
                    "enable-java", false);
                var offscreen = new Gtk.OffscreenWindow ();
                offscreen.add (thumb_view);
                thumb_view.set_size_request (800, 600);
                offscreen.show_all ();
            }

            /* Don't load thumbnails already queued */
            foreach (var spec_ in thumb_queue)
                if (spec_.dial_id == dial_id)
                    return;

            thumb_queue.append (new Spec (dial_id, uri));

            if (thumb_queue.length () > 1){
                return;
            }
            timer_id =  GLib.Timeout.add(1000,check_load_status);
            spec = thumb_queue.nth_data (0);
            thumb_view.load_changed.connect (load_changed);
            thumb_view.load_failed.connect (load_failed);				
            thumb_view.load_uri (spec.uri);
				
#endif
        }
    }
}

