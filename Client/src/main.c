/* 
To compile:
gcc -g main.c cameraAPI.c -o client  -ljpeg -pthread `pkg-config --libs --cflags gtk+-3.0 gmodule-2.0`

Official documentation:
https://developer.gnome.org/gtk3/stable/

*/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "cameraAPI.h"

#define OK 0
#define KO -1

#define DISCONNECTION 0
#define TAKE 1
#define SAVE 2
#define DETECTION 3
#define DETECTED 4
#define NOT_DETECTED 5
#define STREAMING 6
#define STREAMING_ENDED 7


typedef struct {
    GtkWidget *take_button;
    GtkWidget *save_button;
    GtkWidget *detection_button;
    GtkWidget *left_rotation_button;
    GtkWidget *right_rotation_button;
    GtkWidget *start_video_button;
    GtkWidget *stop_video_button;
    GtkWidget *inf_label;
    GtkWidget *main_window;
    GtkWidget *picture;
    GdkPixbuf *pixbuf;
    int rotation_angle;
    pthread_t IPthread;
    pthread_t videoThread;
    int videoStatus;
} IHM;


CAMERA myCam = {0};
IHM myIHM = {0};

void closeAll()
{
    //--------------
    // Function to properly closed the client
    //--------------
    myIHM.videoStatus=0;
    pthread_join(myIHM.videoThread,NULL);
    cameraAPI_destroy(&myCam);

}

void sigint_handler(int sig){
    //--------------
    // Signal Handler
    //--------------
    printf("Signal caught\n");
    closeAll();
    gtk_main_quit();
    exit(0);
}

void format_time(char *output){
    //--------------
    // Format the time to print
    //--------------
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    sprintf(output, "[ %.2dh%.2dm%.2ds ]", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void buttonBlocker(gboolean take, gboolean save, gboolean detect, gboolean rotation, gboolean video)
{
    //--------------
    //Function to enable or disable the GTK's buttons
    //--------------
    if (take) gtk_widget_set_sensitive(myIHM.take_button, TRUE);
    else gtk_widget_set_sensitive(myIHM.take_button, FALSE);
    if (save) gtk_widget_set_sensitive(myIHM.save_button, TRUE);
    else gtk_widget_set_sensitive(myIHM.save_button, FALSE);
    if (detect) gtk_widget_set_sensitive(myIHM.detection_button, TRUE);
    else gtk_widget_set_sensitive(myIHM.detection_button, FALSE);
    if (rotation)
    {
        gtk_widget_set_sensitive(myIHM.left_rotation_button, TRUE);
        gtk_widget_set_sensitive(myIHM.right_rotation_button, TRUE);
    }
    else
    {
        gtk_widget_set_sensitive(myIHM.left_rotation_button, FALSE);
        gtk_widget_set_sensitive(myIHM.right_rotation_button, FALSE);
    }
    if (video)
    {
        gtk_widget_set_sensitive(myIHM.start_video_button, TRUE);
        gtk_widget_set_sensitive(myIHM.stop_video_button, TRUE);
    }
    else
    {
        gtk_widget_set_sensitive(myIHM.start_video_button, FALSE);
        gtk_widget_set_sensitive(myIHM.stop_video_button, FALSE);
    }
}

void updateInfo(int event, char * name)
{
    //--------------
    //Function to update the Information frame in the IHM
    //--------------
    char info[1000];
    char state[250];
    char action[256];
    char time[30];

    format_time(time);

    switch(myCam.status)
    {
    case OK: 
        sprintf(state," • Camera state: <span foreground='green'>Ready</span>\n\n • IP Address: <span foreground='yellow'>%s</span>",myCam.IPaddress); buttonBlocker(TRUE,FALSE,FALSE,FALSE,TRUE);break;
    case KO: sprintf(state," • Camera state: <span foreground='red'>No connection</span>\n\n • IP Address: <span foreground='red'>No connection</span>"); buttonBlocker(FALSE,FALSE,TRUE,FALSE,FALSE);break;
    }

    switch(event)
    {
    case TAKE: sprintf(action, " • Last event:\n\n\t %s <span foreground='blue'>Picture taken</span>", time); buttonBlocker(TRUE,TRUE,FALSE,TRUE,TRUE);break;
    case SAVE: sprintf(action, " • Last event: \n\n\t %s <span foreground='blue'>Picture saved</span>\n\t(%.30s)", time, name);
               if(!myIHM.videoStatus) buttonBlocker(TRUE,TRUE,FALSE,TRUE,TRUE);
               else buttonBlocker(FALSE,TRUE,FALSE,TRUE,TRUE); break;
    case DETECTION: sprintf(action, " • Last event: \n\n\t %s <span foreground='blue'>Detection started</span>", time); buttonBlocker(FALSE,FALSE,FALSE,FALSE,FALSE); break;
    case DETECTED: sprintf(action, " • Last event: \n\n\t %s <span foreground='green'>Camera detected</span>", time); break;
    case NOT_DETECTED: sprintf(action, " • Last event: \n\n\t %s <span foreground='red'>Camera not detected</span>", time); break;
    case STREAMING: sprintf(action, " • Last event: \n\n\t %s <span foreground='red'>REC</span>", time); buttonBlocker(FALSE,TRUE,FALSE,TRUE,TRUE); break;
    case STREAMING_ENDED: sprintf(action, " • Last event: \n\n\t %s <span foreground='blue'>Streaming ended</span>", time);buttonBlocker(TRUE,TRUE,FALSE,TRUE,TRUE); break;
    case DISCONNECTION : sprintf(action, " • Last event:\n\n\t %s <span foreground='red'>Camera Disconnected</span>", time);break;
    default: sprintf(action, " • Last event:\n\n\t\t No event");
    }

    sprintf(info,"%s\n\n%s\n",state,action);
    gtk_label_set_markup(GTK_LABEL(myIHM.inf_label),info);
}

int open_dialog(char * name_file)
{
    //--------------
    //Function that opens a dialog window to select a save file.
    //--------------
    GtkWidget *dialog;

    dialog = gtk_file_chooser_dialog_new("Choose a file:", GTK_WINDOW(myIHM.main_window), 
                         GTK_FILE_CHOOSER_ACTION_SAVE,
                         ("_Cancel"),
                         GTK_RESPONSE_CANCEL,
                         ("_Save"),
                         GTK_RESPONSE_OK, NULL);

    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "image.jpg");
    gtk_widget_show_all(dialog);
    gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
    if (resp == GTK_RESPONSE_OK)
    {
        GError **error = NULL;
        sprintf(name_file,"%s", gtk_file_chooser_get_current_name(GTK_FILE_CHOOSER(dialog)));
        gdk_pixbuf_save(myIHM.pixbuf, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)),"jpeg",error,NULL);
        gtk_widget_destroy(dialog);
        return 1;
    }
    else
    {
        gtk_widget_destroy(dialog);
        return 0;
    }
}

void takePicture(GtkWidget *widget, gpointer data)
{
    //--------------
    //Callback function called when the button 'Take a snapshot' is clicked.
    //--------------
    cameraAPI_snapshot(&myCam);
    if (myCam.status == OK)
    {
        updateInfo(TAKE, NULL);
        GdkPixbufLoader* loader = gdk_pixbuf_loader_new();

        gdk_pixbuf_loader_write(loader, myCam.lastImage, 1000000, NULL);
        myIHM.pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
        myIHM.pixbuf = gdk_pixbuf_rotate_simple(myIHM.pixbuf, myIHM.rotation_angle);
        gtk_image_set_from_pixbuf((GtkImage*) myIHM.picture, myIHM.pixbuf);
    }
    else
    {
        updateInfo(DISCONNECTION, NULL);
    }
}

void* videoStreaming(void* arg)
{
    //--------------
    //Thread that calls the cameraAPI_video function.
    //  Used to display the video stream.
    //--------------

    while(myIHM.videoStatus)
    {
        cameraAPI_video(&myCam, 0);
        if (myCam.status==OK)
        {
            GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
            gdk_pixbuf_loader_write(loader, myCam.lastImage, 1000000, NULL);
            myIHM.pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
            myIHM.pixbuf = gdk_pixbuf_rotate_simple(myIHM.pixbuf, myIHM.rotation_angle);
            gtk_image_set_from_pixbuf((GtkImage*) myIHM.picture, myIHM.pixbuf);
        }
        else
        {
            updateInfo(DISCONNECTION, NULL);
            cameraAPI_destroy(&myCam);
            myIHM.videoStatus = 0;
            return NULL;
        }
    }
    cameraAPI_video(&myCam, 1);
}


void videoStartStream(GtkWidget *widget, gpointer data)
{
    //--------------
    //Callback function called when the button 'Start video' is clicked.
    //--------------
    cameraAPI_video_init(&myCam);
    
    if (myCam.status==OK)
    {
        updateInfo(STREAMING, NULL);
        myIHM.videoStatus = 1;
        pthread_create(&(myIHM.videoThread), NULL, videoStreaming, NULL);
    }
}

void videoStopStream(GtkWidget *widget, gpointer data)
{
    //--------------
    //Callback function called when the button 'Stop video' is clicked.
    //--------------
    myIHM.videoStatus=0;
    pthread_join(myIHM.videoThread,NULL);
    updateInfo(STREAMING_ENDED, NULL);
}

void savePicture(GtkWidget *widget, gpointer data)
{
    //--------------
    //Callback function called when the button 'Save' is clicked.
    // Called the open_dialog function.
    //--------------
    char name_file[256];
    int success = open_dialog(name_file);
    if (success) updateInfo(SAVE, name_file);
}


void* checkIPdetection(void* arg)
{
    //--------------
    //Thread to know when and if the camera is detected.
    //--------------
    pthread_join(myIHM.IPthread, NULL);
    if (myCam.status==0) updateInfo(DETECTED, NULL);
    else updateInfo(NOT_DETECTED, NULL);
}

void cameraDetection(GtkWidget *widget, gpointer data)
{
    //--------------
    //Callback function called when the button 'Detection' is clicked.
    //  Called the cameraAPI_getIP function as thread.
    //--------------
    printf("\nCamera detection started ...\n");
    pthread_t checkIP;
    updateInfo(DETECTION,NULL);
    pthread_create(&(myIHM.IPthread), NULL, cameraAPI_getIP, (void*) &myCam);
    pthread_create(&checkIP, NULL, checkIPdetection, NULL);
}

void leftRotation(GtkWidget *widget, gpointer data)
{
    //--------------
    //Callback function called when the button Left rotation is clicked.
    //--------------
    myIHM.rotation_angle= (myIHM.rotation_angle+90)%360;
    myIHM.pixbuf = gdk_pixbuf_rotate_simple(myIHM.pixbuf, 90);
    gtk_image_set_from_pixbuf((GtkImage*)myIHM.picture, myIHM.pixbuf);
}

void rightRotation(GtkWidget *widget, gpointer data)
{
    //--------------
    //Callback function called when the button Right rotation is clicked.
    //--------------
    myIHM.rotation_angle= (myIHM.rotation_angle+270)%360;
    myIHM.pixbuf = gdk_pixbuf_rotate_simple(myIHM.pixbuf, 270);
    gtk_image_set_from_pixbuf((GtkImage*)myIHM.picture, myIHM.pixbuf);
}


void CSSloader(void){
    //--------------
    //Function that loads the css sheet 
    //--------------
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    provider = gtk_css_provider_new ();
    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    const gchar *myCssFile = "../resources/style.css";
    GError *error = 0;

    gtk_css_provider_load_from_file(provider, g_file_new_for_path(myCssFile), &error);
    g_object_unref (provider);
}

int ihm(int argc, char *argv [])
{
    //--------------
    //Main function of the HMI. Load the glade file and build the HMI.
    //--------------

    GtkWidget * info_frame;
    GtkWidget * camera_detect_grid;
    GtkBuilder *builder = NULL;
    gchar *filename = NULL;
    GError *error = NULL;

    /* Initialisation of the Gtk.library */
    gtk_init(&argc, &argv);


    CSSloader();
    /* Glade file opening*/
    builder = gtk_builder_new();


    /* g_build_filename(): get the path for the file according to the OS */
    filename =  g_build_filename("../resources/IHM.glade", NULL);

    gtk_builder_add_from_file(builder, filename, &error);
    if (error)
    {
        gint code = error->code;
        g_printerr("%s\n", error->message);
        g_error_free (error);
        return code;
    }

    /* Getting of main window pointer */
    myIHM.main_window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    if (NULL == myIHM.main_window)
    {
        /* Print out the error. You can use GLib's message logging  */
        fprintf(stderr, "Unable to file object with id \"window1\" \n");
        /* Your error handling code goes here */
    }
    else
    {
        /* Widget getting */

        myIHM.take_button = GTK_WIDGET(gtk_builder_get_object(builder, "take_button"));
        myIHM.save_button = GTK_WIDGET(gtk_builder_get_object(builder, "save_button"));
        myIHM.detection_button = GTK_WIDGET(gtk_builder_get_object(builder, "detection_button"));
        myIHM.left_rotation_button = GTK_WIDGET(gtk_builder_get_object(builder, "left_rotation_button"));
        myIHM.right_rotation_button = GTK_WIDGET(gtk_builder_get_object(builder, "right_rotation_button"));
        myIHM.start_video_button = GTK_WIDGET(gtk_builder_get_object(builder, "start_video_button"));
        myIHM.stop_video_button = GTK_WIDGET(gtk_builder_get_object(builder, "stop_video_button"));
        myIHM.inf_label = GTK_WIDGET(gtk_builder_get_object(builder, "information_label"));
        myIHM.picture = GTK_WIDGET(gtk_builder_get_object(builder, "picture"));
        info_frame = GTK_WIDGET(gtk_builder_get_object(builder, "info_frame"));
        camera_detect_grid = GTK_WIDGET(gtk_builder_get_object(builder, "camera_detect_grid"));

        /* Widget naming for CSS used */
        gtk_widget_set_name(myIHM.take_button, "take_button");
        gtk_widget_set_name(myIHM.save_button, "save_button");
        gtk_widget_set_name(myIHM.left_rotation_button, "left_rotation_button");
        gtk_widget_set_name(myIHM.right_rotation_button, "right_rotation_button");
        gtk_widget_set_name(myIHM.start_video_button, "start_video_button");
        gtk_widget_set_name(myIHM.stop_video_button, "stop_video_button");
        gtk_widget_set_name(myIHM.main_window, "main_window");
        gtk_widget_set_name(myIHM.detection_button, "detection_button");
        gtk_widget_set_name(myIHM.inf_label, "information_label");
        gtk_widget_set_name(camera_detect_grid, "camera_detect_grid");
        gtk_widget_set_name(info_frame, "info_frame");

        /* Callback function assignment */
        g_signal_connect (G_OBJECT(myIHM.main_window), "destroy", G_CALLBACK(sigint_handler), NULL);
        g_signal_connect(G_OBJECT(myIHM.save_button), "clicked", G_CALLBACK(savePicture), NULL);
        g_signal_connect(G_OBJECT(myIHM.take_button), "clicked", G_CALLBACK(takePicture), NULL);
        g_signal_connect(G_OBJECT(myIHM.detection_button), "clicked", G_CALLBACK(cameraDetection), NULL);
        g_signal_connect(G_OBJECT(myIHM.left_rotation_button), "clicked", G_CALLBACK(leftRotation), NULL);
        g_signal_connect(G_OBJECT(myIHM.right_rotation_button), "clicked", G_CALLBACK(rightRotation), NULL);
        g_signal_connect(G_OBJECT(myIHM.start_video_button), "clicked", G_CALLBACK(videoStartStream), NULL);
        g_signal_connect(G_OBJECT(myIHM.stop_video_button), "clicked", G_CALLBACK(videoStopStream), NULL);

        /* Display the main window */
        gtk_widget_show_all(myIHM.main_window);
        g_free (filename);

        updateInfo(-1, NULL);

        gtk_main();
    }

    return 0;
}

int main(int argc, char *argv [])
{
    signal(SIGINT, sigint_handler);

    myCam.status= KO;

    if (argc == 2){

        strcpy(myCam.IPaddress,argv[1]);
        cameraAPI_init(&myCam);
    }


    printf("API port used : %d\n",PORT);

    ihm(argc, argv);

    closeAll();

    return EXIT_SUCCESS;
}
