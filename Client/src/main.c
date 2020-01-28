/* 
To compile:
gcc -g main.c cameraAPI.c -o client  -ljpeg -pthread `pkg-config --libs --cflags gtk+-3.0 gmodule-2.0`

Official documentation:
https://developer.gnome.org/gtk3/stable/

*/

#include <gtk/gtk.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pthread.h>

#include "cameraAPI.h"

#define OK 0
#define KO -1

#define TAKE 1
#define SAVE 2
#define DETECTION 3
#define DETECTED 4
#define NOT_DETECTED 5

typedef struct {
    GtkWidget *take_button;
    GtkWidget *save_button;
    GtkWidget *detection_button;
    GtkWidget *left_rotation_button;
    GtkWidget *right_rotation_button;
    GtkWidget *inf_label;
    GtkWidget *main_window;
    GtkWidget *picture;
    GdkPixbuf *pixbuf;
    int rotation_angle;
    pthread_t IPthread;
} IHM;


CAMERA myCam = {0};
IHM myIHM = {0};

void sigint_handler(int sig){
    printf("Signal caught\n");
    gtk_main_quit();
    cameraAPI_destroy(&myCam);
    exit(0);
}

void format_time(char *output){
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

/*    sprintf(output, "[%d/%d/%d-%dH:%d':%d'']",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);*/
    sprintf(output, "[ %.2dh%.2dm%.2ds ]", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void buttonBlocker(gboolean take, gboolean save, gboolean detect, gboolean rotation)
{
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
}

void updateInfo(int event, char * name)
{
    char info[1000];
    char state[250];
    char action[256];
    char time[30];

    format_time(time);

    switch(myCam.status)
    {
    case OK: sprintf(state," • Camera state: <span foreground='green'>Ready</span>\n\n • IP Address: %s",myCam.IPaddress); buttonBlocker(TRUE,FALSE,FALSE,FALSE);break;
    case KO: sprintf(state," • Camera state: <span foreground='red'>No connection</span>\n\n • IP Address: No connection"); buttonBlocker(FALSE,FALSE,TRUE,FALSE);break;
    }

    switch(event)
    {
    case TAKE: sprintf(action, " • Last event:\n\n\t %s Picture taken", time); buttonBlocker(TRUE,TRUE,FALSE,TRUE);break;
    case SAVE: sprintf(action, " • Last event: \n\n\t %s Picture saved\n\t(%.30s)", time, name); buttonBlocker(TRUE,TRUE,FALSE,TRUE); break;
    case DETECTION: sprintf(action, " • Last event: \n\n\t %s <span foreground='blue'>Detection started</span>", time); buttonBlocker(FALSE,FALSE,FALSE,FALSE); break;
    case DETECTED: sprintf(action, " • Last event: \n\n\t %s <span foreground='green'>Camera detected</span>", time); break;
    case NOT_DETECTED: sprintf(action, " • Last event: \n\n\t %s <span foreground='red'>Camera not detected</span>", time); break;
    default: sprintf(action, " • Last event:\n\n\t\t No event");
    }

    sprintf(info,"%s\n\n%s\n",state,action);
    gtk_label_set_markup(GTK_LABEL(myIHM.inf_label),info);
}

int open_dialog(char * name_file)
{
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
/*        g_print("\tPath: %s\n", gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));*/

        GError **error = NULL;
        sprintf(name_file,"%s", gtk_file_chooser_get_current_name(GTK_FILE_CHOOSER(dialog)));
        gdk_pixbuf_save(myIHM.pixbuf, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)),"jpeg",error,NULL);
        gtk_widget_destroy(dialog);
        return 1;
    }
    else
    {
/*        g_print("\tSaving cancel\n");*/
        gtk_widget_destroy(dialog);
        return 0;
    }
}

void takePicture(GtkWidget *widget, gpointer data)
{
/*    printf("\nTake picture\n");*/
    cameraAPI_snapshot(&myCam);
    updateInfo(TAKE, NULL);
/*    gtk_image_set_from_file((GtkImage *)picture,"1.png");*/
    GdkPixbufLoader* loader = gdk_pixbuf_loader_new();

    gdk_pixbuf_loader_write(loader, myCam.lastImage, 1000000, NULL);
    myIHM.pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    myIHM.pixbuf = gdk_pixbuf_rotate_simple(myIHM.pixbuf, myIHM.rotation_angle);
    gtk_image_set_from_pixbuf((GtkImage*) myIHM.picture, myIHM.pixbuf);

}


void savePicture(GtkWidget *widget, gpointer data)
{
/*    printf("\nSave picture\n");*/
    char name_file[256];
    int success = open_dialog(name_file);
    if (success) updateInfo(SAVE, name_file);
}


void* checkIPdetection(void* arg)
{
    pthread_join(myIHM.IPthread, NULL);
    if (myCam.status==0) updateInfo(DETECTED, NULL);
    else updateInfo(NOT_DETECTED, NULL);
}

void cameraDetection(GtkWidget *widget, gpointer data)
{
    printf("\nCamera detection started ...\n");
    pthread_t checkIP;
    updateInfo(DETECTION,NULL);
    pthread_create(&(myIHM.IPthread), NULL, cameraAPI_getIP, (void*) &myCam);
    pthread_create(&checkIP, NULL, checkIPdetection, NULL);
}

void leftRotation(GtkWidget *widget, gpointer data)
{
    myIHM.rotation_angle= (myIHM.rotation_angle+90)%360;
    myIHM.pixbuf = gdk_pixbuf_rotate_simple(myIHM.pixbuf, 90);
    gtk_image_set_from_pixbuf((GtkImage*)myIHM.picture, myIHM.pixbuf);
}

void rightRotation(GtkWidget *widget, gpointer data)
{
    myIHM.rotation_angle= (myIHM.rotation_angle+270)%360;
    myIHM.pixbuf = gdk_pixbuf_rotate_simple(myIHM.pixbuf, 270);
    gtk_image_set_from_pixbuf((GtkImage*)myIHM.picture, myIHM.pixbuf);
}


void CSSloader(void){
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    provider = gtk_css_provider_new ();
    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    const gchar *myCssFile = "../resources/mystyle.css";
    GError *error = 0;

    gtk_css_provider_load_from_file(provider, g_file_new_for_path(myCssFile), &error);
    g_object_unref (provider);
}

int ihm(int argc, char *argv [])
{

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
/*    filename =  g_build_filename("../IHM.glade", NULL);*/
    filename =  g_build_filename("../resources/IHM.glade", NULL);

    /* Loading of the "ihm.glade" file */
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
        myIHM.inf_label = GTK_WIDGET(gtk_builder_get_object(builder, "information_label"));
        myIHM.picture = GTK_WIDGET(gtk_builder_get_object(builder, "picture"));
        info_frame = GTK_WIDGET(gtk_builder_get_object(builder, "info_frame"));
        camera_detect_grid = GTK_WIDGET(gtk_builder_get_object(builder, "camera_detect_grid"));


        gtk_widget_set_name(myIHM.take_button, "take_button");
        gtk_widget_set_name(myIHM.save_button, "save_button");
        gtk_widget_set_name(myIHM.left_rotation_button, "left_rotation_button");
        gtk_widget_set_name(myIHM.right_rotation_button, "right_rotation_button");
        gtk_widget_set_name(myIHM.main_window, "main_window");
        gtk_widget_set_name(myIHM.detection_button, "detection_button");
        gtk_widget_set_name(myIHM.inf_label, "information_label");
        gtk_widget_set_name(camera_detect_grid, "camera_detect_grid");
        gtk_widget_set_name(info_frame, "info_frame");

        /* Callback function assignment */
        g_signal_connect (G_OBJECT (myIHM.main_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
        g_signal_connect(G_OBJECT(myIHM.save_button), "clicked", G_CALLBACK(savePicture), NULL);
        g_signal_connect(G_OBJECT(myIHM.take_button), "clicked", G_CALLBACK(takePicture), NULL);
        g_signal_connect(G_OBJECT(myIHM.detection_button), "clicked", G_CALLBACK(cameraDetection), NULL);
        g_signal_connect(G_OBJECT(myIHM.left_rotation_button), "clicked", G_CALLBACK(leftRotation), NULL);
        g_signal_connect(G_OBJECT(myIHM.right_rotation_button), "clicked", G_CALLBACK(rightRotation), NULL);

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

/*    printf("Camera status: %d\n",myCam.status);*/
    ihm(argc, argv);

    cameraAPI_destroy(&myCam);
/*    printf("End\n");*/
    return EXIT_SUCCESS;
}
