#include <cairo/cairo.h>
#include <options/options.h>
#include "../csm/csm_all.h"
#include "../csm/laser_data_drawing.h"

typedef struct {
	/* should we draw it? */
	int draw;
	
	double width;
	const char* color;
	
} line_style ;

typedef struct { 
	line_style rays, countour;
	line_style points;
	double points_radius; 

	line_style normals;
	double normals_length;
	
	/* A circle at the pose */
	line_style pose;
	double pose_radius;
	
	double connect_threshold;
	double horizon;
} ld_style;

void ls_set_defaults(line_style*ls);
void lds_set_defaults(ld_style*lds);

void ls_add_options(line_style*ls, struct option*ops, 
	const char*prefix, const char*desc_prefix);

void lds_add_options(ld_style*lds, struct option*ops, 
	const char*prefix, const char*desc_prefix);

void cr_set_style(cairo_t*cr, line_style*);
void cr_ld_draw(cairo_t* cr, LDP ld, ld_style *p);
void cr_ld_draw_corr(cairo_t*cr, LDP laser_ref, LDP laser_sens, line_style*);

void cr_set_reference(cairo_t*cr,double*pose);

void cr_lda_draw_pose_path(cairo_t*cr, LDP*lda, int nscans, ld_reference use_reference);

int create_pdf_surface(const char*file, int max_width_points, int max_height_points,
	double bb_min[2], double bb_max[2], cairo_surface_t**surface_p, cairo_t **cr);

/** Needs cartesian; returns 0 if not enough points. */
/*int ld_get_bounding_box(LDP ld, double min[2], double max[2], double horizon);*/




