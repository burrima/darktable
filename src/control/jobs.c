/*
		This file is part of darktable,
		copyright (c) 2009--2010 johannes hanika.

		darktable is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation, either version 3 of the License, or
		(at your option) any later version.

		darktable is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "control/jobs.h"
#include "control/conf.h"
#include "common/camera_control.h"
#include "common/darktable.h"
#include "common/image_cache.h"
#include "common/imageio.h"
#include "common/imageio_module.h"
#include "gui/background_jobs.h"
#include "views/view.h"
#include "gui/gtk.h"
#include <glade/glade.h>
#include <glib.h>
#include <glib/gstdio.h>

void dt_image_load_job_init(dt_job_t *job, int32_t id, dt_image_buffer_t mip)
{
	dt_control_job_init(job, "load image %d mip %d", id, mip);
	job->execute = &dt_image_load_job_run;
	dt_image_load_t *t = (dt_image_load_t *)job->param;
	t->imgid = id;
	t->mip = mip;
}

void dt_image_load_job_run(dt_job_t *job)
{
	dt_image_load_t *t = (dt_image_load_t *)job->param;
	dt_image_t *img = dt_image_cache_get(t->imgid, 'r');
	
	char message[512]={0};
	snprintf(message, 512, _("loading image %s"), img->filename);
	const dt_gui_job_t *j = dt_gui_background_jobs_new( DT_JOB_SINGLE, message );
	
	if(!img) return;
	int ret = dt_image_load(img, t->mip);
	// drop read lock, as this is only speculative async loading.
	if(!ret) dt_image_release(img, t->mip, 'r');
	dt_image_cache_release(img, 'r');
	dt_gui_background_jobs_set_progress( j, 1.0 );
}

void dt_captured_image_import_job_run(dt_job_t *job)
{
	dt_captured_image_import_t *t = (dt_captured_image_import_t *)job->param;
	
	char message[512]={0};
	snprintf(message, 512, _("importing image %s"), t->filename);
	const dt_gui_job_t *j = dt_gui_background_jobs_new( DT_JOB_SINGLE, message );
	
	int id = dt_image_import(t->film_id, t->filename);
	if(id)
	{
		//dt_film_open(1);
		dt_view_film_strip_set_active_image(darktable.view_manager,id);
		dt_control_queue_draw_all();
		//dt_ctl_switch_mode_to(DT_DEVELOP);
	}
	dt_gui_background_jobs_set_progress( j , 1.0 );
}

void dt_captured_image_import_job_init(dt_job_t *job,uint32_t filmid, const char *filename)
{
	dt_control_job_init(job, "import tethered image");
	job->execute = &dt_captured_image_import_job_run;
	dt_captured_image_import_t *t = (dt_captured_image_import_t *)job->param;
	t->filename = g_strdup(filename);
	t->film_id = filmid;
}

void dt_camera_capture_job_run(dt_job_t *job)
{
	dt_camera_capture_t *t=(dt_camera_capture_t*)job->param;
	int total = t->count*t->brackets;
	char message[512]={0};
	snprintf(message, 512, ngettext ("capturing %d image", "capturing %d images", total), total );
	double fraction=0;
	const dt_gui_job_t *j = dt_gui_background_jobs_new( DT_JOB_PROGRESS, message );
	for(int i=0;i<t->count;i++)
	{
		
		for(int b=0;b<=t->brackets;b++)
		{
			// if(t->brackets>0 && b==0)
			//  set starting bracket
			// else if( t->brackets>0
			//  set next bracket
			dt_camctl_camera_capture(darktable.camctl,NULL);
			fraction+=1.0/total;
			dt_gui_background_jobs_set_progress( j, fraction);
		}
		
		// Delay if active
		if(t->delay)
			g_usleep(t->delay*G_USEC_PER_SEC);
	 
	}
}

void dt_camera_capture_job_init(dt_job_t *job,uint32_t filmid, uint32_t delay, uint32_t count, uint32_t brackets)
{
	dt_control_job_init(job, "remote capture of image(s)");
	job->execute = &dt_camera_capture_job_run;
	dt_camera_capture_t *t = (dt_camera_capture_t *)job->param;
	t->film_id=filmid;
	t->delay=delay;
	t->count=count;
	t->brackets=brackets;
}

void dt_camera_get_previews_job_init(dt_job_t *job,dt_camera_t *camera,dt_camctl_listener_t *listener,uint32_t flags)
{
	dt_control_job_init(job, "get camera previews job");
	job->execute = &dt_camera_get_previews_job_run;
	dt_camera_get_previews_t *t = (dt_camera_get_previews_t *)job->param;

	t->listener=g_malloc(sizeof(dt_camctl_listener_t));
	memcpy(t->listener,listener,sizeof(dt_camctl_listener_t));
	
	t->camera=camera;
	t->flags=flags;
}

void dt_camera_get_previews_job_run(dt_job_t *job)
{
	dt_camera_get_previews_t *t=(dt_camera_get_previews_t*)job->param;
	
	dt_camctl_register_listener(darktable.camctl,t->listener);
	dt_camctl_get_previews(darktable.camctl,t->flags,t->camera);
	dt_camctl_unregister_listener(darktable.camctl,t->listener);
	g_free(t->listener);
}

void dt_camera_import_backup_job_init(dt_job_t *job,const char *sourcefile, const char *destinationfile)
{
	dt_control_job_init(job, "backup of imported image from camera");
	job->execute = &dt_camera_import_backup_job_run;
	dt_camera_import_backup_t *t = (dt_camera_import_backup_t *)job->param;
	t->sourcefile=g_strdup(sourcefile);
	t->destinationfile=g_strdup(destinationfile);
}

void dt_camera_import_backup_job_run(dt_job_t *job)
{  // copy sourcefile to each found destination
	dt_camera_import_backup_t *t = (dt_camera_import_backup_t *)job->param;
	GVolumeMonitor *vmgr= g_volume_monitor_get();
	GList *mounts=g_volume_monitor_get_mounts(vmgr);
	GMount *mount=NULL;
	GFile *root=NULL;
	if( mounts !=NULL )
		do
		{
		 mount=G_MOUNT(mounts->data);
			if( ( root=g_mount_get_root( mount ) ) != NULL ) 
			{ // Got the mount point lets check for backup folder
				gchar *backuppath=NULL;
				gchar *rootpath=g_file_get_path(root);
				backuppath=g_build_path(G_DIR_SEPARATOR_S,rootpath,dt_conf_get_string("plugins/capture/backup/foldername"),NULL);
				g_free(rootpath);
				
				if( g_file_test(backuppath,G_FILE_TEST_EXISTS)==TRUE)
				{ // Found a backup storage, lets copy file here..
					gchar *destinationfile=g_build_filename(G_DIR_SEPARATOR_S,backuppath,t->destinationfile,NULL);
					if( g_mkdir_with_parents(g_path_get_dirname(destinationfile),0755) >= 0 )
					{            
						gchar *content;
						gsize size;
						if( g_file_get_contents(t->sourcefile,&content,&size,NULL) == TRUE )
						{
							GError *err=NULL;
							if( g_file_set_contents(destinationfile,content,size,&err) != TRUE)
							{
								fprintf(stderr,"Failed to set content of file with reason: %s\n",err->message);
								g_error_free(err);
							}
							g_free(content);
						} 
					}
					g_free(destinationfile);
				}
	
				g_free(backuppath);
			}
		} while( (mounts=g_list_next(mounts)) !=NULL);
		
	// Release volume manager
	g_object_unref(vmgr);
}


void dt_camera_import_job_init(dt_job_t *job,char *jobcode, char *path,char *filename,GList *images, struct dt_camera_t *camera)
{
	dt_control_job_init(job, "import selected images from camera");
	job->execute = &dt_camera_import_job_run;
	dt_camera_import_t *t = (dt_camera_import_t *)job->param;
	dt_variables_params_init(&t->vp);
	t->fraction=0;
	t->images=g_list_copy(images);
	t->camera=camera;
	t->vp->jobcode=g_strdup(jobcode);
	t->path=g_strdup(path);
	t->filename=g_strdup(filename);
	t->import_count=0;
}

/** Listener interface for import job */
void _camera_image_downloaded(const dt_camera_t *camera,const char *filename,void *data)
{
	// Import downloaded image to import filmroll
	dt_camera_import_t *t = (dt_camera_import_t *)data;
	dt_film_image_import(t->film,filename);
	dt_control_log(_("%d/%d imported to %s"), t->import_count+1,g_list_length(t->images), g_path_get_basename(filename));

	t->fraction+=1.0/g_list_length(t->images);
	dt_gui_background_jobs_set_progress( t->bgj, t->fraction );
	
	if( dt_conf_get_bool("plugins/capture/camera/import/backup/enable") == TRUE )
	{ // Backup is enabled, let's initialize a backup job of imported image...
		char *base=dt_conf_get_string("plugins/capture/storage/basedirectory");
		dt_variables_expand( t->vp, base, FALSE );
		const char *sdpart=dt_variables_get_result(t->vp);
		if( sdpart )
		{ // Initialize a image backup job of file
			dt_job_t j;
			dt_camera_import_backup_job_init(&j, filename,filename+strlen(sdpart));
			dt_control_add_job(darktable.control, &j);
		} 
	}
	t->import_count++;
}

const char *_camera_import_request_image_filename(const dt_camera_t *camera,const char *filename,void *data) 
{
	dt_camera_import_t *t = (dt_camera_import_t *)data;
	t->vp->filename=filename;
	
	dt_variables_expand( t->vp, t->path, FALSE );
	const gchar *storage=dt_variables_get_result(t->vp);
	
	dt_variables_expand( t->vp, t->filename, TRUE );
	const gchar *file = dt_variables_get_result(t->vp);
	
	// Start check if file exist if it does, increase sequence and check again til we know that file doesnt exists..
	gchar *fullfile=g_build_path(G_DIR_SEPARATOR_S,storage,file,NULL);
	if( g_file_test(fullfile, G_FILE_TEST_EXISTS) == TRUE )
	{
		do
		{
			g_free(fullfile);
			dt_variables_expand( t->vp, t->filename, TRUE );
			file = dt_variables_get_result(t->vp);
			fullfile=g_build_path(G_DIR_SEPARATOR_S,storage,file,NULL);
		} while( g_file_test(fullfile, G_FILE_TEST_EXISTS) == TRUE);
	}
	
	return file;
}

const char *_camera_import_request_image_path(const dt_camera_t *camera,void *data)
{
	// :) yeap this is kind of stupid yes..
	dt_camera_import_t *t = (dt_camera_import_t *)data;
	return t->film->dirname;
}

void dt_camera_import_job_run(dt_job_t *job)
{
	dt_camera_import_t *t = (dt_camera_import_t *)job->param;
	dt_control_log(_("starting import job of images from camera"));
	
	// Setup a new filmroll to import images to....
	t->film=(dt_film_t*)g_malloc(sizeof(dt_film_t));
	
	dt_film_init(t->film);
	
	dt_variables_expand( t->vp, t->path, FALSE );
	sprintf(t->film->dirname,"%s",dt_variables_get_result(t->vp));
	
	pthread_mutex_lock(&t->film->images_mutex);
	t->film->ref++;
	pthread_mutex_unlock(&t->film->images_mutex);
	
	// Create recursive directories, abort if no access
	if( g_mkdir_with_parents(t->film->dirname,0755) == -1 )
	{
		dt_control_log(_("failed to create import path %s, import of images aborted."), t->film->dirname);
		return;
	}
	
	// Import path is ok, lets actually create the filmroll in database..
	if(dt_film_new(t->film,t->film->dirname) > 0)
	{
		int total = g_list_length( t->images );
		char message[512]={0};
		sprintf(message, ngettext ("importing %d image from camera", "importing %d images from camera", total), total );
		t->bgj = dt_gui_background_jobs_new( DT_JOB_PROGRESS, message);
		
		// Switch to new filmroll
		dt_film_open(t->film->id);
		dt_ctl_switch_mode_to(DT_LIBRARY);
		
		// register listener 
		dt_camctl_listener_t listener= {0};
		listener.data=t;
		listener.image_downloaded=_camera_image_downloaded;
		listener.request_image_path=_camera_import_request_image_path;
		listener.request_image_filename=_camera_import_request_image_filename;
		
		//  start download of images
		dt_camctl_register_listener(darktable.camctl,&listener);
		dt_camctl_import(darktable.camctl,t->camera,t->images,dt_conf_get_bool("plugins/capture/camera/import/delete_originals"));
		dt_camctl_unregister_listener(darktable.camctl,&listener);
		
		dt_variables_params_destroy(t->vp);

	}
	else
		dt_control_log(_("failed to create filmroll for camera import, import of images aborted."));
	
	pthread_mutex_lock(&t->film->images_mutex);
	t->film->ref--;
	pthread_mutex_unlock(&t->film->images_mutex);
}


void dt_film_import1_init(dt_job_t *job, dt_film_t *film)
{
	dt_control_job_init(job, "cache load raw images for preview");
	job->execute = &dt_film_import1_run;
	dt_film_import1_t *t = (dt_film_import1_t *)job->param;
	t->film = film;
	pthread_mutex_lock(&film->images_mutex);
	film->ref++;
	pthread_mutex_unlock(&film->images_mutex);
}

void dt_film_import1_run(dt_job_t *job)
{
	dt_film_import1_t *t = (dt_film_import1_t *)job->param;
	dt_film_import1(t->film);
	pthread_mutex_lock(&t->film->images_mutex);
	t->film->ref--;
	pthread_mutex_unlock(&t->film->images_mutex);
	if(t->film->ref <= 0)
	{
		dt_film_cleanup(t->film);
		free(t->film);
	}
}

// ====================
// develop:
// ====================

void dt_dev_raw_load_job_run(dt_job_t *job)
{
	dt_dev_raw_load_t *t = (dt_dev_raw_load_t *)job->param;
	dt_dev_raw_load(t->dev, t->image);
}

void dt_dev_raw_load_job_init(dt_job_t *job, dt_develop_t *dev, dt_image_t *image)
{
	dt_control_job_init(job, "develop load raw image %s", image->filename);
	job->execute =&dt_dev_raw_load_job_run;
	dt_dev_raw_load_t *t = (dt_dev_raw_load_t *)job->param;
	t->dev = dev;
	t->image = image;
}

void dt_dev_process_preview_job_run(dt_job_t *job)
{
	dt_dev_process_t *t = (dt_dev_process_t *)job->param;
	dt_dev_process_preview_job(t->dev);
}

void dt_dev_process_preview_job_init(dt_job_t *job, dt_develop_t *dev)
{
	dt_control_job_init(job, "develop process preview");
	job->execute = &dt_dev_process_preview_job_run;
	dt_dev_process_t *t = (dt_dev_process_t *)job->param;
	t->dev = dev;
}

void dt_dev_process_image_job_run(dt_job_t *job)
{
	dt_dev_process_t *t = (dt_dev_process_t *)job->param;
	dt_dev_process_image_job(t->dev);
}

void dt_dev_process_image_job_init(dt_job_t *job, dt_develop_t *dev)
{
	dt_control_job_init(job, "develop image preview");
	job->execute = &dt_dev_process_image_job_run;
	dt_dev_process_t *t = (dt_dev_process_t *)job->param;
	t->dev = dev;
}

typedef struct dt_control_image_enumerator_t
{
	GList *index;
}
dt_control_image_enumerator_t;

void dt_control_write_dt_files_job_run(dt_job_t *job)
{
	long int imgid = -1;
	dt_control_image_enumerator_t *t1 = (dt_control_image_enumerator_t *)job->param;
	GList *t = t1->index;
	while(t)
	{
		imgid = (long int)t->data;
		dt_image_t *img = dt_image_cache_use(imgid, 'r');
		char dtfilename[520];
		dt_image_full_path(img, dtfilename, 512);
		char *c = dtfilename + strlen(dtfilename);
		sprintf(c, ".dt");
		dt_imageio_dt_write(imgid, dtfilename);
		sprintf(c, ".dttags");
		dt_imageio_dttags_write(imgid, dtfilename);
		dt_image_cache_release(img, 'r');
		t = g_list_delete_link(t, t);
	}
}

void dt_control_duplicate_images_job_run(dt_job_t *job)
{
	long int imgid = -1;
	dt_control_image_enumerator_t *t1 = (dt_control_image_enumerator_t *)job->param;
	GList *t = t1->index;
	int total = g_list_length(t);
	char message[512]={0};
	double fraction=0;
	snprintf(message, 512, ngettext ("duplicating %d image", "duplicating %d images", total), total );
	const dt_gui_job_t *j = dt_gui_background_jobs_new( DT_JOB_PROGRESS, message);
	while(t)
	{
		imgid = (long int)t->data;
		dt_image_duplicate(imgid);
		t = g_list_delete_link(t, t);
		fraction=1.0/total;
		dt_gui_background_jobs_set_progress(j, fraction);
	}
}

void dt_control_remove_images_job_run(dt_job_t *job)
{
	long int imgid = -1;
	dt_control_image_enumerator_t *t1 = (dt_control_image_enumerator_t *)job->param;
	GList *t = t1->index;
	int total = g_list_length(t);
	char message[512]={0};
	double fraction=0;
	snprintf(message, 512, ngettext ("removing %d image", "removing %d images", total), total );
	const dt_gui_job_t *j = dt_gui_background_jobs_new( DT_JOB_PROGRESS, message);
	while(t)
	{
		imgid = (long int)t->data;
		dt_image_remove(imgid);
		t = g_list_delete_link(t, t);
		fraction=1.0/total;
		dt_gui_background_jobs_set_progress(j, fraction);
	}
}

void dt_control_delete_images_job_run(dt_job_t *job)
{
	long int imgid = -1;
	dt_control_image_enumerator_t *t1 = (dt_control_image_enumerator_t *)job->param;
	GList *t = t1->index;
	int total = g_list_length(t);
	char message[512]={0};
	double fraction=0;
	snprintf(message, 512, ngettext ("deleting %d image", "deleting %d images", total), total );
	const dt_gui_job_t *j = dt_gui_background_jobs_new(DT_JOB_PROGRESS, message);
	while(t)
	{
		imgid = (long int)t->data;
		dt_image_t *img = dt_image_cache_use(imgid, 'r');
		char dtfilename[512];
		dt_image_full_path(img, dtfilename, 512);
		int rc;
		sqlite3_stmt *stmt;
		// remove from db:
		rc = sqlite3_prepare_v2(darktable.db, "delete from history where imgid = ?1", -1, &stmt, NULL);
		rc = sqlite3_bind_int (stmt, 1, imgid);
		rc = sqlite3_step(stmt);
		rc = sqlite3_finalize (stmt);
		rc = sqlite3_prepare_v2(darktable.db, "delete from images where id = ?1", -1, &stmt, NULL);
		rc = sqlite3_bind_int (stmt, 1, imgid);
		rc = sqlite3_step(stmt);
		rc = sqlite3_finalize (stmt);
		rc = sqlite3_prepare_v2(darktable.db, "delete from mipmaps where imgid = ?1", -1, &stmt, NULL);
		rc = sqlite3_bind_int (stmt, 1, imgid);
		rc = sqlite3_step(stmt);
		rc = sqlite3_finalize (stmt);
		rc = sqlite3_prepare_v2(darktable.db, "delete from selected_images where imgid = ?1", -1, &stmt, NULL);
		rc = sqlite3_bind_int (stmt, 1, imgid);
		rc = sqlite3_step(stmt);
		rc = sqlite3_finalize (stmt);
		// remove from disk:
		(void)g_unlink(dtfilename);
		char *c = dtfilename + strlen(dtfilename);
		sprintf(c, ".dt");
		(void)g_unlink(dtfilename);
		sprintf(c, ".dttags");
		(void)g_unlink(dtfilename);
		dt_image_cache_release(img, 'r');
		t = g_list_delete_link(t, t);
		fraction=1.0/total;
		dt_gui_background_jobs_set_progress(j, fraction);
	}
}

void dt_control_image_enumerator_job_init(dt_control_image_enumerator_t *t)
{
	t->index = NULL;
	int rc;
	sqlite3_stmt *stmt;
	rc = sqlite3_prepare_v2(darktable.db, "select * from selected_images order by rowid desc", -1, &stmt, NULL);
	while(sqlite3_step(stmt) == SQLITE_ROW)
	{
		long int imgid = sqlite3_column_int(stmt, 0);
		t->index = g_list_prepend(t->index, (gpointer)imgid);
	}
	sqlite3_finalize(stmt);
}

void dt_control_write_dt_files_job_init(dt_job_t *job)
{
	dt_control_job_init(job, "write dt files");
	job->execute = &dt_control_write_dt_files_job_run;
	dt_control_image_enumerator_t *t = (dt_control_image_enumerator_t *)job->param;
	dt_control_image_enumerator_job_init(t);
}

void dt_control_write_dt_files()
{
	dt_job_t j;
	dt_control_write_dt_files_job_init(&j);
	dt_control_add_job(darktable.control, &j);
}

void dt_control_duplicate_images_job_init(dt_job_t *job)
{
	dt_control_job_init(job, "duplicate images");
	job->execute = &dt_control_duplicate_images_job_run;
	dt_control_image_enumerator_t *t = (dt_control_image_enumerator_t *)job->param;
	dt_control_image_enumerator_job_init(t);
}

void dt_control_remove_images_job_init(dt_job_t *job)
{
	dt_control_job_init(job, "remove images");
	job->execute = &dt_control_remove_images_job_run;
	dt_control_image_enumerator_t *t = (dt_control_image_enumerator_t *)job->param;
	dt_control_image_enumerator_job_init(t);
}

void dt_control_delete_images_job_init(dt_job_t *job)
{
	dt_control_job_init(job, "delete images");
	job->execute = &dt_control_delete_images_job_run;
	dt_control_image_enumerator_t *t = (dt_control_image_enumerator_t *)job->param;
	dt_control_image_enumerator_job_init(t);
}

void dt_control_duplicate_images()
{
	dt_job_t j;
	dt_control_duplicate_images_job_init(&j);
	dt_control_add_job(darktable.control, &j);
}

void dt_control_remove_images()
{
	if(dt_conf_get_bool("ask_before_remove"))
	{
		GtkWidget *dialog;
		GtkWidget *win = glade_xml_get_widget (darktable.gui->main_window, "main_window");
		dialog = gtk_message_dialog_new(GTK_WINDOW(win),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				_("do you really want to remove all selected images from the collection?"));
		gtk_window_set_title(GTK_WINDOW(dialog), _("remove images?"));
		gint res = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(res != GTK_RESPONSE_YES) return;
	}
	dt_job_t j;
	dt_control_remove_images_job_init(&j);
	dt_control_add_job(darktable.control, &j);
}

void dt_control_delete_images()
{
	if(dt_conf_get_bool("ask_before_delete"))
	{
		GtkWidget *dialog;
		GtkWidget *win = glade_xml_get_widget (darktable.gui->main_window, "main_window");
		dialog = gtk_message_dialog_new(GTK_WINDOW(win),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				_("do you really want to physically delete all selected images from disk?"));
		gtk_window_set_title(GTK_WINDOW(dialog), _("delete images?"));
		gint res = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if(res != GTK_RESPONSE_YES) return;
	}
	dt_job_t j;
	dt_control_delete_images_job_init(&j);
	dt_control_add_job(darktable.control, &j);
}

void dt_control_export_job_run(dt_job_t *job)
{
	long int imgid = -1;
	dt_control_image_enumerator_t *t1 = (dt_control_image_enumerator_t *)job->param;
	GList *t = t1->index;
	const int total = g_list_length(t);
	dt_imageio_module_format_t  *mformat  = dt_imageio_get_format();
	g_assert(mformat);
	dt_imageio_module_data_t *fdata = mformat->get_params(mformat);
	fdata->max_width  = dt_conf_get_int ("plugins/lighttable/export/width");
	fdata->max_height = dt_conf_get_int ("plugins/lighttable/export/height");
	dt_imageio_module_storage_t *mstorage = dt_imageio_get_storage();
	g_assert(mstorage);
	
	// Get max dimensions...
	uint32_t w,h,fw,fh,sw,sh;
	w=h=fw=fh=sw=sh=0; // We are all equals!!!
	mstorage->dimension(mstorage, &sw,&sh);
	mformat->dimension(mformat, &fw,&fh);

	if( sw==0 || fw==0) w=sw>fw?sw:fw;
	else w=sw<fw?sw:fw;

	if( sh==0 || fh==0) h=sh>fh?sh:fh;
	else h=sh<fh?sh:fh;

	// fprintf(stderr,"fmw=%d fmh=%d w=%d h=%d\n",fdata->max_width,fdata->max_height,w,h);
	fdata->max_width = (w!=0 && fdata->max_width >w)?w:fdata->max_width;
	fdata->max_height = (h!=0 && fdata->max_height >h)?h:fdata->max_height;
	
	dt_imageio_module_data_t *sdata = mstorage->get_params(mstorage);
	if( sdata == NULL || fdata == NULL ) {
		if( sdata ) {
			mstorage->free_params(mstorage,sdata);
			dt_control_log(_("failed to get parameters from format module, aborting export.."));
		} else {
			mformat->free_params (mformat,  fdata);
			dt_control_log(_("failed to get parameters from storage module, aborting export.."));
		}
		return;
	}
	dt_control_log(ngettext ("exporting %d image..", "exporting %d images..", total), total);
	char message[512]={0};
	snprintf(message, 512, ngettext ("exporting %d image to %s", "exporting %d images to %s", total), total, mstorage->name() );
	const dt_gui_job_t *j = dt_gui_background_jobs_new( DT_JOB_PROGRESS, message );
	double fraction=0;
	while(t)
	{
		imgid = (long int)t->data;
		t = g_list_delete_link(t, t);
		mstorage->store(sdata, imgid, mformat, fdata, total-g_list_length(t), total);
		fraction+=1.0/total;
		dt_gui_background_jobs_set_progress( j, fraction );
	}
	mformat->free_params (mformat,  fdata);
	mstorage->free_params(mstorage, sdata);
}

void dt_control_export_job_init(dt_job_t *job)
{
	dt_control_job_init(job, "export");
	job->execute = &dt_control_export_job_run;
	dt_control_image_enumerator_t *t = (dt_control_image_enumerator_t *)job->param;
	dt_control_image_enumerator_job_init(t);
}

void dt_control_export()
{
	dt_job_t j;
	dt_control_export_job_init(&j);
	dt_control_add_job(darktable.control, &j);
}



