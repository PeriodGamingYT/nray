//// includes
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

//// settings
#define RAYTRACE_MAX 1000
#define RAYTRACE_DEPTH_MAX 3
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define VIEWPORT_WIDTH 1
#define VIEWPORT_HEIGHT 1
#define PROJECTION_PLANE_D 1
typedef long double num;

//// vectors
struct vector3 {
	num x;
	num y;
	num z;
};

#define VECTOR3_INIT(arg_x, arg_y, arg_z) \
	{ \
		.x = arg_x, \
		.y = arg_y, \
		.z = arg_z \
	}

#define VECTOR3_INIT_NUM(n) \
	VECTOR3_INIT(n, n, n)

num vector3_dot(
	struct vector3 a,
	struct vector3 b
) {
	return (
		a.x * b.x +
		a.y * b.y +
		a.z * b.z
	);
}

num vector3_length(
	struct vector3 a
) {
	return (num)sqrt(
		a.x * a.x +
		a.y * a.y +
		a.z * a.z
	);
}

#define DECL_VECTOR3_OPERATION(name, op) \
	struct vector3 name( \
		struct vector3 a, \
		struct vector3 b \
	) { \
		struct vector3 result = VECTOR3_INIT( \
			a.x op b.x, \
			a.y op b.y, \
			a.z op b.z \
		); \
	  \
		return result; \
	} \

DECL_VECTOR3_OPERATION(vector3_add, +)
DECL_VECTOR3_OPERATION(vector3_sub, -)
DECL_VECTOR3_OPERATION(vector3_mul, *)
DECL_VECTOR3_OPERATION(vector3_div, /)

//// scene
struct sphere {
	struct vector3 color;
	struct vector3 center;
	num radius;
	num specular;
	num reflective;
};

struct scene {
	struct sphere *spheres;
	int sphere_length;
	struct light *lights;
	int light_length;
	struct vector3 camera_position;
	struct vector3 camera_rotation;
};

struct closest_return {
	num closest_t;
	struct sphere *closest_sphere;	
};

//// spheres
struct intersect_return {
	num t0;
	num t1;	
};

struct intersect_return intersect_ray_sphere(
	struct vector3 origin,
	struct vector3 d,
	struct sphere arg_sphere
) {
	struct intersect_return inf_t = {
		.t0 = RAYTRACE_MAX,
		.t1 = RAYTRACE_MAX
	};
	
	num r = arg_sphere.radius;
	struct vector3 co = vector3_sub(
		origin, 
		arg_sphere.center
	);

	num a = vector3_dot(d, d);
	num b = 2 * vector3_dot(co, d);
	num c = vector3_dot(co, co) - r * r;
	num discriminant = (b * b) - (4 * a * c);
	if(discriminant < 0) {
		return inf_t;
	}

	struct intersect_return t;
	t.t0 = (-b + (num)sqrt(discriminant)) / (2 * a);
	t.t1 = (-b - (num)sqrt(discriminant)) / (2 * a);
	return t;
}

//// lighting
enum light_type {
	POINT = 0,
	DIRECTIONAL,
	AMBIENT
};

struct light {
	enum light_type type;
	struct vector3 pos;
	num intensity;
	num reflective;
};

struct closest_return closest_intersection(
	struct vector3 origin,
	struct vector3,
	num,
	num,
	struct scene *
);

num compute_lighting(
	struct vector3 pos,
	struct vector3 normal,
	struct vector3 v,
	num specular,
	num t_max,
	struct scene *config
) {
	num intensity = 0;
	for(int i = 0; i < config->light_length; i++) {
		struct vector3 l = VECTOR3_INIT_NUM(0);
		switch(config->lights[i].type) {
			case AMBIENT:
				intensity += config->lights[i].intensity;
				continue;

			case DIRECTIONAL:
				l = config->lights[i].pos;
				break;

			case POINT:
				l = vector3_sub(config->lights[i].pos, pos);
				break;

			default:
				break;
		}

		struct closest_return shadow = closest_intersection(
			pos,
			l,
			0.001,
			t_max,
			config
		);

		struct sphere *shadow_sphere = shadow.closest_sphere;
		if(shadow_sphere != NULL) {
			continue;
		}
		
		num n_dot_l = vector3_dot(normal, l);
		if(n_dot_l > 0) {
			intensity += (
				config->lights[i].intensity *
				n_dot_l / (
					vector3_length(normal) *
					vector3_length(l)
				)
			);
		}

		if(specular != -1) {
			struct vector3 n_dot_l_vec = VECTOR3_INIT_NUM(n_dot_l);
			struct vector3 two_vec = VECTOR3_INIT_NUM(2);
			struct vector3 r = vector3_mul(two_vec, normal);
			r = vector3_mul(r, n_dot_l_vec);
			r = vector3_sub(r, l);

			num r_dot_v = vector3_dot(r, v);
			if(r_dot_v > 0) {
				intensity += (
					config->lights[i].intensity *
					pow(
						r_dot_v /
						(
							vector3_length(r) *
							vector3_length(v)
						),

						specular
					)
				);
			}
		}
	}

	// Intensity sometimes goes above 1.
	// If that happens, the color will go above 255.
	// When that happens, SDL will overflow back
	// to zero.
	return intensity < 1 ? intensity : 1;
}

//// raytracing
#define X_IN(x, a, b) (((x) >= (a)) && ((x) <= (b)))
struct closest_return closest_intersection(
	struct vector3 origin,
	struct vector3 d,
	num t_min,
	num t_max,
	struct scene *config
) {
	num closest_t = RAYTRACE_MAX;
	struct sphere *closest_sphere = NULL;
	for(int i = 0; i < config->sphere_length; i++) {
		struct intersect_return t = intersect_ray_sphere(
			origin,
			d,
			config->spheres[i]
		);

		if(X_IN(t.t0, t_min, t_max) && t.t0 < closest_t) {
			closest_t = t.t0;
			closest_sphere = &config->spheres[i];
		}

		if(X_IN(t.t1, t_min, t_max) && t.t1 < closest_t) {
			closest_t = t.t1;
			closest_sphere = &config->spheres[i];
		}
	}
	
	struct closest_return result = {
		.closest_t = closest_t,
		.closest_sphere = closest_sphere
	};

	return result;
}

struct vector3 reflect_ray(
	struct vector3 ray,
	struct vector3 normal
) {
	struct vector3 two_vec = VECTOR3_INIT_NUM(2);
	struct vector3 result = vector3_mul(
		two_vec,
		normal
	);

	num n_dot_r = vector3_dot(normal, ray);
	struct vector3 n_dot_r_vec = VECTOR3_INIT_NUM(n_dot_r);
	result = vector3_mul(result, n_dot_r_vec);
	result = vector3_sub(result, ray);
	return result;
}

struct vector3 trace_ray(
	struct vector3 origin,
	struct vector3 d,
	num t_min,
	num t_max,
	struct scene *config,
	int depth
) {	
	struct closest_return closest = closest_intersection(
		origin,
		d,
		t_min,
		t_max,
		config
	);

	num closest_t = closest.closest_t;
	struct sphere *closest_sphere = closest.closest_sphere;
	if(closest_sphere == NULL) {
		struct vector3 background_color = VECTOR3_INIT_NUM(0);
		return background_color;
	}

	struct vector3 closest_t_vec = VECTOR3_INIT_NUM(closest_t);
	struct vector3 pos = vector3_mul(
		vector3_add(
			origin, 
			closest_t_vec
		), 
		
		d
	);
	
	struct vector3 normal = vector3_sub(pos, closest_sphere->center);
	num n_length = vector3_length(normal);
	struct vector3 n_length_vec = VECTOR3_INIT_NUM(n_length);
	normal = vector3_div(normal, n_length_vec);
	struct vector3 neg_one_vec = VECTOR3_INIT_NUM(-1);
	struct vector3 neg_d = vector3_mul(d, neg_one_vec);
	num computed = compute_lighting(
		pos, 
		normal,
		neg_d,
		closest_sphere->specular,
		t_max,
		config
	);

	struct vector3 computed_vec = VECTOR3_INIT_NUM(computed);
	struct vector3 local_color = vector3_mul(
		closest_sphere->color, 
		computed_vec
	);

	num r = closest_sphere->reflective;
	if(
		depth >= RAYTRACE_DEPTH_MAX ||
		r <= 0
	) {
		return local_color;
	}

	struct vector3 ray = reflect_ray(neg_d, normal);
	struct vector3 reflected_color = trace_ray(
		pos,
		ray,
		0.001,
		RAYTRACE_MAX,
		config,
		depth + 1
	);

	num one_sub_r = 1 - r;
	struct vector3 one_sub_r_vec = VECTOR3_INIT_NUM(one_sub_r);
	struct vector3 result = vector3_mul(
		local_color,
		one_sub_r_vec
	);

	struct vector3 r_vec = VECTOR3_INIT_NUM(r);
	result = vector3_add(
		result, 
		vector3_mul(
			reflected_color,
			r_vec
		)
	);

	return result;
}

struct vector3 screen_to_viewport(
	num x,
	num y
) {
	struct vector3 result = VECTOR3_INIT(
		x * VIEWPORT_WIDTH / SCREEN_WIDTH,
		y * VIEWPORT_HEIGHT / SCREEN_HEIGHT,
		PROJECTION_PLANE_D	
	);

	return result;
}

void raytrace_with_renderer(
	struct scene *config,
	SDL_Renderer *renderer
) {
	for(
		num x = -SCREEN_WIDTH / 2; 
		x < SCREEN_WIDTH / 2; 
		x++
	) {
		for(
			num y = -SCREEN_HEIGHT / 2;
			y < SCREEN_HEIGHT / 2;
			y++
		) {
			struct vector3 d = vector3_mul(
				config->camera_rotation,
				screen_to_viewport(x, y)
			);

			struct vector3 color = trace_ray(
				config->camera_position,
				d,
				1,
				RAYTRACE_MAX,
				config,
				0
			);
			
			SDL_SetRenderDrawColor(
				renderer,
				(int)color.x,
				(int)color.y,
				(int)color.z,
				255
			);

			SDL_RenderDrawPoint(
				renderer, 
				(int)(x + (SCREEN_WIDTH / 2)), 

				// With x and y correction, y is upside
				// down so I need to correct for that.
				(int)(SCREEN_HEIGHT - (y + (SCREEN_HEIGHT / 2))) 
			);
		}
	}

	SDL_RenderPresent(renderer);
}

//// main
void die(char *function) {
	fprintf(stderr, "%s(): %s\n", function, SDL_GetError());
	exit(-1);
}

static SDL_Window *window;
static SDL_Renderer *renderer;
void quit_sdl() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
int main() {
	if(SDL_Init(SDL_INIT_VIDEO) == -1) {
		die("SDL_Init");
	}

	atexit(quit_sdl);
	SDL_CreateWindowAndRenderer(
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
		0,
		&window,
		&renderer
	);

	if(window == NULL || renderer == NULL) {
		die("SDL_CreateWindowAndRenderer");
	}

	struct sphere config_spheres[] = {
		{
			.color = VECTOR3_INIT(255, 255, 0),
			.center = VECTOR3_INIT(0, -101, 0),
			.radius = 100,
			.specular = 0,
			.reflective = 0
		},

		{
			.color = VECTOR3_INIT(0, 255, 255),
			.center = VECTOR3_INIT(0, -5002, 0),
			.radius = 5000,
			.specular = 0.5,
			.reflective = 0
		},

		{
			.color = VECTOR3_INIT(222, 184, 135),
			.center = VECTOR3_INIT(0.2, -0.2, 1),
			.radius = 0.05,
			.specular = 0.8,
			.reflective = 0.05
		},

		{
			.color = VECTOR3_INIT(222, 184, 135),
			.center = VECTOR3_INIT(0.23, -0.12, 1),
			.radius = 0.05,
			.specular = 0.8,
			.reflective = 0.05
		},

		{
			.color = VECTOR3_INIT(222, 184, 135),
			.center = VECTOR3_INIT(0.24, -0.04, 1),
			.radius = 0.05,
			.specular = 0.8,
			.reflective = 0.05
		},

		{
			.color = VECTOR3_INIT(222, 184, 135),
			.center = VECTOR3_INIT(0.22, 0.04, 1),
			.radius = 0.05,
			.specular = 0.8,
			.reflective = 0.05
		},

		{
			.color = VECTOR3_INIT(222, 184, 135),
			.center = VECTOR3_INIT(0.2, 0.12, 1),
			.radius = 0.05,
			.specular = 0.8,
			.reflective = 0.05
		},

		{
			.color = VECTOR3_INIT(0, 255, 0),
			.center = VECTOR3_INIT(0.22, 0.32, 1),
			.radius = 0.2,
			.specular = 0.8,
			.reflective = 0.1
		}
	};

	struct light config_lights[] = {
		{
			.type = AMBIENT,
			.intensity = 0.2,
			.pos = VECTOR3_INIT(0, 0, 0)
		},

		{
			.type = POINT,
			.intensity = 0.6,
			.pos = VECTOR3_INIT(2, 1, 0)
		},

		{
			.type = DIRECTIONAL,
			.intensity = 0.2,
			.pos = VECTOR3_INIT(1, 4, 4)
		}
	};
	
	struct scene config = {
		.spheres = config_spheres,
		.sphere_length = ARRAY_SIZE(config_spheres),
		.lights = config_lights,
		.light_length = ARRAY_SIZE(config_lights),
		.camera_position = VECTOR3_INIT(0, 0, 0),

		// NOTE: Must be above one on all coordinates.
		.camera_rotation = VECTOR3_INIT(1, 1, 1)
	};

	raytrace_with_renderer(&config, renderer);
	int is_keep_open = 1;
	SDL_Event event;
	while(
		SDL_PollEvent(&event) != -1 && 
		is_keep_open
	) {
		switch(event.type) {
			case SDL_QUIT:
				is_keep_open = 0;
				break;

			default:
				break;
		}
	}
	
	exit(0);
}
