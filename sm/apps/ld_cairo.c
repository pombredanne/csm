
#include <string.h>
#include "ld_cairo.h"

void cr_ld_draw_rays(cairo_t*, LDP);
void cr_ld_draw_countour(cairo_t*, LDP, double, double);
void cr_ld_draw_points(cairo_t*, LDP, double radius);

void cr_ld_draw_corr(cairo_t*cr, LDP laser_ref, LDP laser_sens, line_style*ls) {
	int i;
	for(i=0; i < laser_sens->nrays; i++) {
		if(!ld_valid_corr(laser_sens,i)) continue;

		if(!laser_sens->corr[i].valid) continue;

		int j1 = laser_sens->corr[i].j1;
		int j2 = laser_sens->corr[i].j2;
		
		const double *p_j1  = laser_ref->points[j1].p;
		const double *p_j2  = laser_ref->points[j2].p;
		const double *p_i_w = laser_sens->points_w[i].p;
		double proj[2];
		projection_on_line_d(p_j1,  p_j2, p_i_w, proj, 0);
	
		cairo_move_to(cr, p_i_w[0], p_i_w[1]);
/*		cairo_line_to(cr, p_j1[0], p_j1[1]);*/
		cairo_line_to(cr, proj[0],  proj[1]);
		cairo_stroke(cr);	
	}
}

const char* cat(const char*a, const char*b) {
	size_t la = strlen(a);
	size_t lb = strlen(b);
	char* buf = malloc(la+lb+3);
	stpcpy(stpcpy(buf, a), b);
	return buf;
}

void ls_add_options(line_style*ls, struct option*ops, 
	const char*prefix, const char*desc_prefix) 
{
	options_int(ops, cat(prefix, "draw"), &(ls->draw), 
		ls->draw, cat(desc_prefix, "Whether to draw it (0,1)"));
	
	options_string(ops, cat(prefix, "color"), &(ls->color), 
		ls->color, cat(desc_prefix, "Color ('red', '#f00')"));

	options_double(ops, cat(prefix, "width"), &(ls->width), 
		ls->width, cat(desc_prefix, "line width (meters)"));
		
}


void lds_add_options(ld_style*lds, struct option*ops, 
	const char*prefix, const char*desc_prefix) 
{
	ls_add_options(&(lds->rays), ops, cat(prefix, "rays_"),  cat(desc_prefix, "Rays | "));
	ls_add_options(&(lds->countour), ops, cat(prefix, "countour_"),  cat(desc_prefix, "Countour | "));
	ls_add_options(&(lds->points), ops, cat(prefix, "points_"),  cat(desc_prefix, "Points | "));
	
	options_double(ops, cat(prefix, "points_radius"), &(lds->points_radius), 
		lds->points_radius, cat(desc_prefix, "Point radius"));

	ls_add_options(&(lds->normals), ops, cat(prefix, "normals_"),  cat(desc_prefix, "Normals | "));

	options_double(ops, cat(prefix, "normals_length"), &(lds->points_radius), 
		lds->points_radius, cat(desc_prefix, "Length of normals sticks (meters)"));

	options_double(ops, cat(prefix, "connect_threshold"), &(lds->connect_threshold), 
		lds->connect_threshold, cat(desc_prefix, "Threshold under which points are connected (m)."));
	options_double(ops, cat(prefix, "horizon"), &(lds->horizon), 
		lds->horizon, cat(desc_prefix, "Maximum distance to plot (m)."));
}

void cr_set_style(cairo_t*cr, line_style*ls) {
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	
	cairo_set_line_width(cr, ls->width);
}

int ld_get_bounding_box(LDP ld, double min[2], double max[2], double horizon) {
	int nrays_used = 0;
	int first=1;
	int i; for(i=0;i<ld->nrays;i++) {
		if(!ld->valid[i]) continue;
		if(ld->readings[i]>horizon) continue;

		double *p = ld->points[i].p;

		if(first) {
			min[0]=max[0]=p[0];
			min[1]=max[1]=p[1];
			first = 0;
		} else {
			min[0] = GSL_MIN(min[0], p[0]);
			min[1] = GSL_MIN(min[1], p[1]);
			max[0] = GSL_MAX(max[0], p[0]);
			max[1] = GSL_MAX(max[1], p[1]);
		}
		nrays_used++;
	}
	return nrays_used > 4;
}


void cr_ld_draw_rays(cairo_t*cr, LDP ld) {
	int i; for(i=0;i<ld->nrays;i++) {
		if(!ld_valid_ray(ld, i)) continue;
		
		double threshold = 2;
		if(ld->readings[i]<2) continue;
		
		double x1 = threshold * cos(ld->theta[i]);
		double y1 = threshold * sin(ld->theta[i]);
		double x2 = ld->readings[i] * cos(ld->theta[i]);
		double y2 = ld->readings[i] * sin(ld->theta[i]);
		
		cairo_move_to(cr,x1,y1);
		cairo_line_to(cr,x2,y2);
		cairo_stroke(cr);
	}
}

struct stroke_sequence {
	int begin_new_stroke;
	int end_stroke;
	int valid;
	
	double p[2];
}; 

void compute_stroke_sequence(LDP ld, struct stroke_sequence*draw_info,
	double horizon, double connect_threshold) {
	int last_valid = -1; int first = 1;
	int i; for(i=0;i<ld->nrays;i++) {
		if( (!ld_valid_ray(ld,i)) || (ld->readings[i] > horizon) ) {
			draw_info[i].valid = 0;
			continue;
		}
		draw_info[i].valid = 1;
		draw_info[i].p[0] = ld->readings[i] * cos(ld->theta[i]);
		draw_info[i].p[1] = ld->readings[i] * sin(ld->theta[i]);
		
		if(first) { 
			first = 0; 
			draw_info[i].begin_new_stroke = 1;
			draw_info[i].end_stroke = 0;
		} else {
			int near =  square(connect_threshold) > 
				distance_squared_d(draw_info[last_valid].p, draw_info[i].p);
			draw_info[i].begin_new_stroke = near ? 0 : 1;
			draw_info[i].end_stroke = 0;
			draw_info[last_valid].end_stroke = draw_info[i].begin_new_stroke;
		}
		last_valid = i;
	} /*for */
	if(last_valid >= 0)
		draw_info[last_valid].end_stroke = 1;
} /* find buff .. */

void cr_ld_draw_countour(cairo_t*cr, LDP ld, double horizon, double connect_threshold) {
	struct stroke_sequence draw_info[ld->nrays];
	compute_stroke_sequence(ld, draw_info, horizon, connect_threshold);
	
	/* draw contour: begin_new_stroke and end_stroke tell 
	when to interrupt the stroke */
	int i; 
	for(i=0;i<ld->nrays;i++) {

		if(draw_info[i].valid==0) continue;

		double *p = draw_info[i].p;
		
		if(draw_info[i].begin_new_stroke)
			cairo_move_to(cr, p[0], p[1]);
		else
			cairo_line_to(cr, p[0], p[1]);
		if(draw_info[i].end_stroke)
			cairo_stroke(cr);
	}
}

void cr_ld_draw_points(cairo_t*cr, LDP ld, double radius) {
	int i; for(i=0;i<ld->nrays;i++) {
		if(!ld_valid_ray(ld, i)) continue;

		double x = ld->readings[i] * cos(ld->theta[i]);
		double y = ld->readings[i] * sin(ld->theta[i]);

		cairo_arc (cr, x, y, radius, 0.0, 2*M_PI);
		cairo_stroke (cr);
	}
}

void cr_ld_draw_normals(cairo_t*cr, LDP ld, double length) {
	int i; for(i=0;i<ld->nrays;i++) {
		if(!ld_valid_ray(ld, i) || !ld_valid_alpha(ld, i)) continue;

		double alpha = ld->alpha[i];
		double x1 = ld->readings[i] * cos(ld->theta[i]);
		double y1 = ld->readings[i] * sin(ld->theta[i]);
		double x2 = x1 + cos(alpha) * length;
		double y2 = x2 + sin(alpha) * length;

		cairo_move_to(cr, x1, y1);
		cairo_line_to(cr, x2, y2);
		cairo_stroke (cr);
	}
}

void cr_ld_draw(cairo_t* cr, LDP ld, ld_style *p) {
	if(p->rays.draw) {
		cr_set_style(cr, &(p->rays));
		cr_ld_draw_rays(cr, ld);
	}
		
	if(p->countour.draw)  {
		cr_set_style(cr, &(p->countour));
		cr_ld_draw_countour(cr, ld, p->horizon, p->connect_threshold);
	}

	if(p->points.draw) {
		cr_set_style(cr, &(p->points));
		cr_ld_draw_points(cr, ld, p->points_radius);
	}

	if(p->normals.draw) {
		cr_set_style(cr, &(p->normals));
		cr_ld_draw_normals(cr, ld, p->normals_length);
	}
}

void cr_set_reference(cairo_t*cr, double*pose) {
	cairo_rotate(cr,pose[2]);
	cairo_translate(cr,pose[0],pose[1]);
}

void ls_set_defaults(line_style*ls) {
	ls->draw = 1;
	ls->color = "black";
	ls->width = 0.002;
}

void lds_set_defaults(ld_style*lds) {
	ls_set_defaults(&(lds->rays));
	lds->rays.color = "#f00";
	lds->rays.width = 0.0002;
	
	ls_set_defaults(&(lds->countour));
	ls_set_defaults(&(lds->points));
	lds->points_radius = 0.003;
	lds->points.color = "#f00";
	
	lds->normals_length = 0.10;
	ls_set_defaults(&(lds->normals));
	
	lds->connect_threshold = 0.20;
	lds->horizon = 10;
}

