#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

/*
	include swarm & netstat, the fileman should be included too. ~RD
*/

/*  GUI.c
 *	Author: insert name.
 *	
 *  Refactor: Robin Duda. ~RD
 *	
 *  Merged duplicate functions and extended usability.
 *	Now able to use torrent data in tables.
 *  Sorts torrents based on state at additions.
 *  Identifies selected torrents with tab id and list id.
 *  Updates displayed torrent data in separate thread.
 *  Compressed column creating function.
 *  Removed static addition of rows, added runtime addition and deletion. (used for full delete, and state moves)
 *  Added more columns and made it easier to manage them.
 *  Turned tab 'all' into tab 'inactive', to avoid redundancy and increase readability and simplicity.
 *  Rows are moved between tabs on user interaction. (start/stop/remove)
 *  Todo: sorting on column click.
 *  Todo: adding torrent from path/patch checking for new files?
 *  Todo: add drag and drop? insert with path reference. (HOT)
 *  Todo: move rows between tabs on completion/queue-process.
 *  Todo: add queue handling.
 *  Todo: separate more code from the core GUI. (data loaders, anything not reliant on GTK)
 *  Todo: queue handling should work from the incomplete when there are slots in downloading. 
 */

 //developer note, gtk get requests for integers takes a integer reference, not a pointer.

//todo: decrease redundancy, turn all tab into inactive tab.
//todo: add more tabs? log, peers, trackers? ~RD
//todo: 

//add tab: seeding, peers, trackers, torrentinfo, logs? ~RD
#define TORRENT_TABS 4
#define TAB_DOWNLOADING 1
#define TAB_SEEDING 2
#define TAB_COMPLETED 3
#define TAB_INACTIVE 4
#define true 1
#define false 0
#define FPS 8

 //following values is a part of the prioritizer. ~RD
 #define MAX_DOWNLOADING 3
 #define MAX_SEEDING 20

 enum
 {
 	STATE_DOWNLOADING,
 	STATE_SEEDING,
 	STATE_COMPLETED,
 	STATE_INACTIVE,
 };

enum {
	COL_ID = 0,
	COL_NAME = 1,
	COL_SIZE = 2,
	COL_DONE = 3,
	COL_STATUS = 4,
	COL_DOWNRATE = 5,
	COL_UPRATE = 6,
	COL_LEECHER = 7,
	COL_SEEDER = 8,
	COL_SWARM = 9,
	COL_RATIO = 10,
};

//torrent data to display, load from includes. ~RD
typedef struct
{
	int id;				//used to identify actions on a torrent.
	char* size;
	char* done;
	char* status;
	char* name;
	char* info_hash;	//hidden attribute used for updating data
	char* path;			//hidden attribute used for double-click.
} torrentlist_t;		//sort the list to implement priority.


//to add a column: increase the COUNT and add a NAME. ~RD
//in list_create add your data-type.
//in update_list to refresh the value.
//in torrentlist?
static int COLUMN_COUNT = 11;
static char* COLUMN_NAME[] = {"#", "Name", "Size", "Done", "Status", "Download", "Upload", "Leeches", "Seeds", "Swarm", "Ratio"};

//dynamic array of torrentlist.  ~RD
torrentlist_t* torrentlist; 
int torrentlist_count;
pthread_t update_thread;

//global required, multiple pointers to retain references. ~RD
GtkWidget *tv_inactive, *tv_downloading, *tv_completed, *tv_seeding;
GtkWidget *tlb_inactive, *tlb_downloading, *tlb_completed, *tlb_seeding;
GtkListStore *md_inactive, *md_downloading, *md_completed, *md_seeding;
GtkWidget *lb_netstat;
GtkWidget *notebook;
double pc = 0.00;

// set rotation of a meter
void set_meter(int m, int percent, GdkPixbuf *pbuf){
	static int current_deg[4]; 
	int to_add;

	GdkPixbuf *tmp;
	//GtkWidget *meter;

	to_add = (percent - current_deg[m])*1.8;
	tmp = pbuf;

	while(to_add>0){
		pbuf = gdk_pixbuf_rotate_simple(pbuf, to_add);
		to_add -= 90;
	}

	g_object_unref(tmp);
	//meter = gtk_image_new_from_pixbuf(pbuf);
	pbuf = gdk_pixbuf_rotate_simple(pbuf, to_add);
	g_object_unref(tmp);
	//meter = gtk_image_new_from_pixbuf(pbuf);
	current_deg[m] = percent;
}

//add a row when the torrent-info already exists. ~RD
void row_add(int id, GtkListStore* ls)
{
	GtkTreeIter iter;

   	gtk_list_store_append(ls, &iter);
   	gtk_list_store_set(ls, &iter,
   						COL_ID, id, 
   						COL_NAME, torrentlist[id].name, 
   						COL_SIZE, torrentlist[id].size, 
   						COL_DONE, torrentlist[id].done, 
   						COL_STATUS, torrentlist[id].status, 
   						COL_DOWNRATE, "N/A", 
   						COL_UPRATE, "N/A", 		//get up/downrate from netstat.c ~RD
   						COL_LEECHER, 0, 
   						COL_SEEDER, 0, 
   						COL_SWARM, 0, 			//get these values from swarm.c ~RD
   						COL_RATIO, 0.0000, -1);
}

//remove a row when the torrent-info already exists. ~RD
void row_delete(int id, GtkListStore* ls)
{
    GtkTreeIter  iter;
    gboolean     nextitem_exist;
    int item_id;

	nextitem_exist = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls), &iter);

    while (nextitem_exist)
 	{
     	gtk_tree_model_get(GTK_TREE_MODEL(ls), &iter, COL_ID, &item_id, -1);

     	if (id == item_id)
     	{
     		printf("\nMatch for deletion.");
     		gtk_list_store_remove(ls, &iter);
     	}

        nextitem_exist = gtk_tree_model_iter_next(GTK_TREE_MODEL(ls), &iter);
	}   
}

//add an info-item to list. ~RD
void list_add(char* name, char* status, char* size, char* done, char* info_hash, int state)
{

	if ((torrentlist = realloc(torrentlist, sizeof(torrentlist_t) * (torrentlist_count + 1))) != NULL )
	{
		torrentlist[torrentlist_count].size = malloc(8);
		torrentlist[torrentlist_count].done = malloc(8);
		torrentlist[torrentlist_count].status = malloc(16);
		torrentlist[torrentlist_count].name = malloc(32);
		torrentlist[torrentlist_count].info_hash = malloc(21);

		strcpy(torrentlist[torrentlist_count].info_hash, info_hash);
		strcpy(torrentlist[torrentlist_count].size, size);
		strcpy(torrentlist[torrentlist_count].done, done);
		strcpy(torrentlist[torrentlist_count].status, status);
		strcpy(torrentlist[torrentlist_count].name, name);
		torrentlist[torrentlist_count].id = torrentlist_count;

		torrentlist_count++;
	}

   	switch (state)
   	{
   		case STATE_COMPLETED: 	row_add(torrentlist_count-1, md_completed); break;
   		case STATE_SEEDING: 	row_add(torrentlist_count-1, md_seeding); break;
   		case STATE_DOWNLOADING: row_add(torrentlist_count-1, md_downloading); break;
   		case STATE_INACTIVE:	row_add(torrentlist_count-1, md_inactive); break;
   	}
}

//update torrent item in liststore tab. This functions is EXTREMELY expensive. ~RD
void list_update(GtkListStore *ls)
{
    GtkTreeIter  iter;
    gboolean     nextitem_exist;
    char percent[20];

	sprintf(percent, "%.2f %%", pc);
	nextitem_exist = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ls), &iter);

    while (nextitem_exist)
 	{
    	//todo: fetch actual values, percent, peercount, swarm-size, leeches, seeders.
    	//get id in column here, from use torrent-info to get data from modules.
       gtk_list_store_set(ls, &iter, COL_DONE, percent, -1);
       nextitem_exist = gtk_tree_model_iter_next(GTK_TREE_MODEL(ls), &iter);
	}
}


//update the torrents with data from modules at defined FPS. ~RD
void* gui_update_thread(void* arg)
{
	int pgnum;

	while (true)
	{
		usleep((1000 / FPS) * 1000);
		pc += 0.01;

		//todo: fetch actual values.
		//optimization: only update the active tab.
		pgnum = (int) gtk_notebook_get_current_page((GtkNotebook*) notebook);
		switch (pgnum)
		{
			case TAB_INACTIVE: 		list_update(md_inactive);			break;
			case TAB_DOWNLOADING:	list_update(md_downloading);	break;
			case TAB_COMPLETED: 	list_update(md_completed); 		break;
			case TAB_SEEDING:	 	list_update(md_seeding); 		break;
		}

		gtk_label_set_text((GtkLabel*) lb_netstat, "U: 279.8 kB/s D: 844.2 kB/s");		//todo: get speeds from netstat.c
		//check if some items needs to be moved.
	}
}

//create a list model by reference. ~RD
void list_create(GtkListStore **model)
{
	*model = gtk_list_store_new(COLUMN_COUNT, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
								G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_DOUBLE); 
}

//handle doubleclick event on torrent. ~RD
void list_doubleclick(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
	GtkTreeIter   iter;
	GtkTreeModel *model;
	int id;

	model = gtk_tree_view_get_model(view);

	if (gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter, COL_ID, &id, -1);

		printf("\nDouble clicking [%s].", torrentlist[id].info_hash);
		fflush(stdout);
		//fork and spawn nautilus with directory from top level file.
		//the directory should be found in the file-manager. bencodning.c?
	}
}

//return selected tab as id. ~RD
int selected_tab(void)
{
	return (int) gtk_notebook_get_current_page((GtkNotebook*) notebook);
}

//returns the torrentlist-id of the selected torrent in the selected tab. ~RD
int selected_id(void)
{
	//check call to gtk_tree_model_get and fix the pointer *id which is leaking memory.
	GtkTreeSelection* tsel;
	GtkTreeIter iter;
    GtkTreeModel* tm;
    GtkTreeModel* md;
    //long int* id = malloc(8);
    long int id;  
	int pgnum = selected_tab();

	switch (pgnum)
	{
		case TAB_INACTIVE: 		tsel 	= gtk_tree_view_get_selection((GtkTreeView*) tv_inactive); 
					 			md 		= (GtkTreeModel*) md_inactive; 
					  			 break;
		case TAB_DOWNLOADING: 	tsel 	= gtk_tree_view_get_selection((GtkTreeView*) tv_downloading); 
					  			md 		= (GtkTreeModel*) md_downloading; 
					  			 break;
		case TAB_COMPLETED:   	tsel 	= gtk_tree_view_get_selection((GtkTreeView*) tv_completed); 
					  			md 		= (GtkTreeModel*) md_completed; 
					  			 break;
		case TAB_SEEDING:     	tsel  	= gtk_tree_view_get_selection((GtkTreeView*) tv_seeding);
							   	md      = (GtkTreeModel*) md_seeding;
							     break;
	}

	if (0 < pgnum && pgnum <= TORRENT_TABS)
    	if (gtk_tree_selection_get_selected(tsel, &tm, &iter))
    	{
      		gtk_tree_model_get(md, &iter, COL_ID, &id, -1);
      		return (long int) id;
    	}
    return -1;
}

//if valid id is selected and valid tab (torrent tab) selected. ~RD
void torrent_start()
{
	int id, tab = selected_tab();
	if ((id = selected_id()) < 0)
		return;

	switch(tab)
	{
		case TAB_SEEDING: 	row_delete(id, md_seeding);
							row_add(id, md_completed);
							 break;
		case TAB_COMPLETED: row_delete(id, md_completed);
							row_add(id, md_seeding); 
							 break;
		case TAB_INACTIVE:  row_delete(id, md_inactive);
							//todo add check if torrent is done or not, if done then seed, if not done then download.
							row_add(id, md_downloading);
							//track(torrentlist[id].info_hash); //get the trackerlist from file man.
							break;
	}

	printf("\nStarted torrent %s.", torrentlist[id].info_hash);
	fflush(stdout);
}

//if valid id is selected and valid tab (torrent tab) selected. ~RD
void torrent_stop()
{
	int id, tab = selected_tab();
	if ((id = selected_id()) < 0)
		return;

	switch (tab)
	{
		case TAB_SEEDING: 		row_delete(id, md_seeding); 
						  		row_add(id, md_completed); 
						  		//untrack(torrentlist[id].info_hash); 
						  		break;

		case TAB_DOWNLOADING: 	row_delete(id, md_downloading);
								row_add(id, md_inactive);
							  	//untrack(torrentlist[id].info_hash); 
							  	break;
	}
	
	printf("\nStopped torrent %s.", torrentlist[id].info_hash);
	fflush(stdout);
}

//if valid id is selected and valid tab (torrent tab) selected. ~RD
//remove object from all tabs.
void torrent_delete()
{
	int id, tab = selected_tab();
	if ((id = selected_id()) < 0)
		return;

	switch (tab)
	{
		case TAB_SEEDING: 		row_delete(id, md_seeding); break;
		case TAB_DOWNLOADING: 	row_delete(id, md_downloading); break;
		case TAB_COMPLETED: 	row_delete(id, md_completed); break;
		case TAB_INACTIVE:	 	row_delete(id, md_inactive); break;
	}

	//todo: delete torrent files && delete torrentinfo record?

	printf("\nDeleted torrent %s.", torrentlist[id].info_hash);
	fflush(stdout);
}

//keep id intact, reorder list. ~RD
void torrent_prioritize()
{
	int id;
	if ((id = selected_id()) < 0)
		return;

	g_print ("You clicked on the arrow up button!\n");
	fflush(stdout);
}

//keep id intact, reorder list. ~RD
void torrent_deprioritize()
{
	int id;
	if ((id = selected_id()) < 0)
		return;

	g_print ("You clicked on the arrow down button!\n");
	fflush(stdout);
}

void torrent_create(){
	g_print("Create button woop!\n");
	fflush(stdout);	
}


void MOTD(GtkWidget **label, GtkWidget **table) {
	*label = gtk_label_new("MOTD goes here."); // Label content
  	gtk_misc_set_alignment(GTK_MISC (*label), 0, 1); // Sets alignment of label
  	gtk_table_attach_defaults (GTK_TABLE (*table), *label, 0, 1, 10, 11); // Sets beginning position of label in table
}

void netstat_label(GtkWidget **label, GtkWidget **table) {
	*label = gtk_label_new("Upload/Download speed");
  	gtk_misc_set_alignment(GTK_MISC (*label), 1, 1);
  	gtk_table_attach_defaults(GTK_TABLE (*table), *label, 5, 6, 10, 11);
}

void create_notebook (GtkWidget **table, GtkWidget **notebook) {
	*notebook = gtk_notebook_new(); // Creates new notebook
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(*notebook), GTK_POS_TOP); // Sets tab position
	gtk_table_attach_defaults(GTK_TABLE(*table), *notebook, 0,6,1,10); // Sets row and columns for notebook
}

void create_table (GtkWidget **window, GtkWidget **table) {
	*table = gtk_table_new(11,6,TRUE); // Creates rows and columns for table
	gtk_container_add(GTK_CONTAINER(*window), *table); // Adds table to main window
	gtk_table_set_row_spacing(GTK_TABLE(*table),0,5); // Sets row space on row 5
}

void static enum_list(GtkWidget **tree_view, GtkListStore **model, GtkTreeViewColumn **column) {
	GtkCellRenderer   *renderer;

	*tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(*model));
	int i;

	for (i = 0; i < COLUMN_COUNT; i++)
	{
		renderer = gtk_cell_renderer_text_new();
		*column = gtk_tree_view_column_new_with_attributes(COLUMN_NAME[i], renderer, "text", i, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(*tree_view), *column);
	}
	g_object_unref(*model);
}

void create_home (GtkWidget **label, GtkWidget **home_table, GtkWidget **view, GtkWidget **notebook) {
	*label = gtk_label_new("Home"); // Tab name
	*home_table = gtk_table_new(1,3,TRUE);
	*view = gtk_label_new("RSS Table"); // Content of "Home" tab
	gtk_widget_set_usize(*view, 300, 30); // Max WIDTH x HEIGHT of content in tab
	gtk_misc_set_alignment(GTK_MISC(*view), 0, 0); // X & Y alignment of content
	gtk_misc_set_padding(GTK_MISC(*view), 10, 10); // Left/Right & Top/Bottom padding
	gtk_table_attach_defaults(GTK_TABLE(*home_table), *view, 2, 3, 0, 1);
	*view = gtk_label_new("Counters Table"); // Content of "Home" tab
	gtk_widget_set_usize(*view, 300, 30); // Max WIDTH x HEIGHT of content in tab
	gtk_misc_set_alignment(GTK_MISC(*view), 0, 0); // X & Y alignment of content
	gtk_misc_set_padding(GTK_MISC(*view), 10, 10); // Left/Right & Top/Bottom padding
	gtk_table_attach_defaults(GTK_TABLE(*home_table), *view, 0, 2, 0, 1);
	gtk_notebook_insert_page(GTK_NOTEBOOK(*notebook), *home_table, *label, 0); // Position of tab, in this case it's first
}

//create a new torrent-tab with name and pos specified. ~RD
void create_torrent_tab(GtkWidget **label, GtkWidget **scrolled_window, GtkWidget **notebook, GtkWidget **treeView, char* name, int pos)
{
	*label = gtk_label_new(name);
	*scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(*scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC); // X & Y scroll set to automatic
	gtk_container_add(GTK_CONTAINER(*scrolled_window), *treeView);
	gtk_notebook_insert_page (GTK_NOTEBOOK (*notebook), *scrolled_window, *label, pos); // Adds scrolled window to tab and positions it
}

void create_menu (GtkWidget **toolbar, GtkWidget **table) {
	GtkToolItem	*play;
	GtkToolItem	*stop;
	GtkToolItem	*delete;
	GtkToolItem	*up;
	GtkToolItem	*down;
	GtkToolItem *create;

	*toolbar = gtk_toolbar_new(); // Creates new toolbar menu
  	gtk_toolbar_set_style(GTK_TOOLBAR(*toolbar), GTK_TOOLBAR_ICONS); 	// Sets style to display icons only
	gtk_table_attach_defaults (GTK_TABLE (*table), *toolbar, 0,2,0,1);  // Sets beginning position of toolbar

	//create the menu buttons.
	play = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY); // Declares "PLAY" button
  	gtk_toolbar_insert(GTK_TOOLBAR(*toolbar), play, 0); // Adds "PLAY" button to the toolbar

	stop = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
  	gtk_toolbar_insert(GTK_TOOLBAR(*toolbar), stop, 1);

	delete = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
  	gtk_toolbar_insert(GTK_TOOLBAR(*toolbar), delete, 2);

	up = gtk_tool_button_new_from_stock(GTK_STOCK_GO_UP);
  	gtk_toolbar_insert(GTK_TOOLBAR(*toolbar), up, 3);

	down = gtk_tool_button_new_from_stock(GTK_STOCK_GO_DOWN);
  	gtk_toolbar_insert(GTK_TOOLBAR(*toolbar), down, 4);

  	create = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_toolbar_insert(GTK_TOOLBAR(*toolbar), create, 5);

  	//add event handlers for the buttons.
	g_signal_connect(G_OBJECT(play), "clicked", G_CALLBACK(torrent_start), NULL);
	g_signal_connect(G_OBJECT(stop), "clicked", G_CALLBACK(torrent_stop), NULL);
	g_signal_connect(G_OBJECT(delete), "clicked", G_CALLBACK(torrent_delete), NULL);
	g_signal_connect(G_OBJECT(up), "clicked", G_CALLBACK(torrent_prioritize), NULL);
	g_signal_connect(G_OBJECT(down), "clicked", G_CALLBACK(torrent_deprioritize), NULL);
	g_signal_connect(G_OBJECT(create), "clicked", G_CALLBACK(torrent_create), NULL);

	g_signal_connect(tv_inactive, "row-activated", G_CALLBACK(list_doubleclick), NULL);
	g_signal_connect(tv_completed, "row-activated", G_CALLBACK(list_doubleclick), NULL);
	g_signal_connect(tv_downloading, "row-activated", G_CALLBACK(list_doubleclick), NULL);
	g_signal_connect(tv_seeding, "row-activated", G_CALLBACK(list_doubleclick), NULL);
}

int main (int argc, char *argv[])
{
	GtkWidget		*window;
	GtkWidget		*view;
	GtkWidget		*toolbar;
	GtkWidget		*table;
	GtkWidget		*home_table;
	GtkWidget		*label;
	GtkWidget		*scrolled_window;
	GtkTreeViewColumn 	*column;

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL); // Creates main window
	gtk_window_set_title(GTK_WINDOW(window), "Torrent"); // Title of main window
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER); // Main window is centered on start
	gtk_container_border_width (GTK_CONTAINER (window), 10);// Inner border of window is set to 10
	gtk_widget_set_app_paintable(window, TRUE);

	create_table(&window, &table);
	create_notebook(&table, &notebook);

// --------------List some torrents in TreeViews.----------------------------- ~RD
	torrentlist_count = 0;

	create_home(&label, &view, &home_table, &notebook);

	list_create(&md_inactive);
	list_create(&md_seeding);
	list_create(&md_completed);
	list_create(&md_downloading);

	enum_list(&tv_downloading, &md_downloading, &column);
	create_torrent_tab(&tlb_downloading, &scrolled_window, &notebook, &tv_downloading, "Downloading", 1);

	enum_list(&tv_seeding, &md_seeding, &column);
	create_torrent_tab(&tlb_seeding, &scrolled_window, &notebook, &tv_seeding, "Seeding", 2);	

	enum_list(&tv_completed, &md_completed, &column);
	create_torrent_tab(&tlb_completed, &scrolled_window, &notebook, &tv_completed, "Completed", 3);

	enum_list(&tv_inactive, &md_inactive, &column);
	create_torrent_tab(&tlb_inactive, &scrolled_window, &notebook, &tv_inactive, "Inactive", 4);

	//dynamic adding at runtime. State is defined by the torrent-loader, unfinished torrents should always be placed in downloading
	//keep track of which torrents are in seeding/completed-mode in config file. Also keep track of priorities in settings file.
	//the setting file should be compiled whenever there are changes to a state in torrents. (moved by user) ~RD
	list_add("Photoflop CS7", 			"Completed", 	"8.43 GB", 	 "100.00%", "----  INFOHASH  ----", STATE_COMPLETED);
	list_add("World of Catcraft", 		"Downloading", 	"12.47 GB",  "68.13%",  "----  INFOHASH  ----", STATE_DOWNLOADING);
	list_add("The.Shrimpsons S08E03", 	"Downloading", 	"413.89 MB", "12.04%",  "----  INFOHASH  ----", STATE_DOWNLOADING);
	list_add("EBook_ASM_Cookbook", 		"Downloading", 	"55.10 MB",  "97.89%",  "----  INFOHASH  ----", STATE_DOWNLOADING);
	list_add("The.Shrimpsons S08E04", 	"Seeding", 	"374.95 MB", "100.00%", "----  INFOHASH  ----", STATE_SEEDING);
	list_add("The.Shrimpsons S08E05", 	"Completed", 	"415.10 MB", "100.00%", "----  INFOHASH  ----", STATE_COMPLETED);
//---------------------------------------------------------------------------  ~RD

	MOTD(&label, &table);
	netstat_label(&lb_netstat, &table);
	create_menu(&toolbar, &table);

// Show window widget and it's child widgets
	gtk_widget_show_all(window);
	g_signal_connect_swapped(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	if (!(pthread_create(&update_thread, NULL, gui_update_thread, NULL)))
			printf("\nUpdating your values in Thread.");

	gtk_main();
	return 0;
}