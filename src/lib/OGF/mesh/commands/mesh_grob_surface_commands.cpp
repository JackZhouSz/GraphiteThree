/*
 *  OGF/Graphite: Geometry and Graphics Programming Library + Utilities
 *  Copyright (C) 2000-2009 INRIA - Project ALICE
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  If you modify this software, you should include a notice giving the
 *  name of the person performing the modification, the date of modification,
 *  and the reason for such modification.
 *
 *  Contact: Bruno Levy - levy@loria.fr
 *
 *     Project ALICE
 *     LORIA, INRIA Lorraine, 
 *     Campus Scientifique, BP 239
 *     54506 VANDOEUVRE LES NANCY CEDEX 
 *     FRANCE
 *
 *  Note that the GNU General Public License does not permit incorporating
 *  the Software into proprietary programs. 
 *
 * As an exception to the GPL, 
 *   Graphite can be linked with the following (non-GPL) libraries:
 *     Qt, SuperLU, WildMagic and CGAL
 */

#include <OGF/mesh/commands/mesh_grob_surface_commands.h>

#include <geogram/mesh/mesh_baking.h>
#include <geogram/image/image_library.h>
#include <geogram/image/morpho_math.h>


#ifdef GEOGRAM_WITH_VORPALINE
#include <vorpalib/mesh/mesh_remesh.h>
#include <vorpalib/mesh/mesh_quaddom.h>
#endif

#include <geogram/mesh/mesh_preprocessing.h>
#include <geogram/mesh/mesh_repair.h>
#include <geogram/mesh/mesh_fill_holes.h>
#include <geogram/mesh/mesh_remesh.h>
#include <geogram/mesh/mesh_geometry.h>
#include <geogram/mesh/mesh_intersection.h>
#include <geogram/mesh/mesh_decimate.h>
#include <geogram/mesh/mesh_AABB.h>
#include <geogram/mesh/mesh_subdivision.h>
#include <geogram/mesh/mesh_smoothing.h>
#include <geogram/parameterization/mesh_atlas_maker.h>
#include <geogram/parameterization/mesh_param_packer.h>
#include <geogram/parameterization/mesh_LSCM.h>
#include <geogram/parameterization/mesh_ABF.h>
#include <geogram/basic/command_line.h>

#include <OGF/scene_graph/types/scene_graph.h>

namespace {
    using namespace OGF;

    /**
     * \brief Post-processes the result of a boolean operation.
     * \details Triangulates the facets, collapses the small edges
     *  and removes self-intersections.
     */
    void fix_mesh_for_boolean_ops(Mesh* M) {
	GEO::mesh_repair(
	    *M,
	    GEO::MeshRepairMode(
		GEO::MESH_REPAIR_COLOCATE | GEO::MESH_REPAIR_DUP_F
	    ),
	    1e-3*GEO::surface_average_edge_length(*M)
	);
	GEO::tessellate_facets(*M,3);
	GEO::mesh_remove_intersections(*M);	
    }
}

namespace OGF {
    
    MeshGrobSurfaceCommands::MeshGrobSurfaceCommands() { 
    }

    MeshGrobSurfaceCommands::~MeshGrobSurfaceCommands() { 
    }
    
    void MeshGrobSurfaceCommands::merge_vertices(
        double epsilon
    ) {
        epsilon *= (0.01 * bbox_diagonal(*mesh_grob()));
        mesh_repair(
	    *mesh_grob(),
	    GEO::MeshRepairMode(
		GEO::MESH_REPAIR_COLOCATE | GEO::MESH_REPAIR_DUP_F
	    ),
	    epsilon
	);
        mesh_grob()->update();
    }

    void MeshGrobSurfaceCommands::repair_surface(
        double epsilon,
        double min_comp_area,
        double max_hole_area,
        double max_hole_edges,
        double max_degree3_dist,
        bool remove_intersections
    ) {
        double bbox_diagonal = GEO::bbox_diagonal(*mesh_grob());
        epsilon *= (0.01 * bbox_diagonal);
        double area = GEO::Geom::mesh_area(*mesh_grob(),3);
        min_comp_area *= area;
        max_hole_area *= area;

        mesh_repair(*mesh_grob(), GEO::MESH_REPAIR_DEFAULT, epsilon);

        if(min_comp_area != 0.0) {
            double nb_f_removed = mesh_grob()->facets.nb();
            GEO::remove_small_connected_components(*mesh_grob(), min_comp_area);
            nb_f_removed -= mesh_grob()->facets.nb();
            if(nb_f_removed != 0) {
                GEO::mesh_repair(
                    *mesh_grob(), GEO::MESH_REPAIR_DEFAULT, epsilon
                );
            }
        }

        if(max_hole_area != 0.0 && max_hole_edges != 0) {
            GEO::fill_holes(
                *mesh_grob(), max_hole_area, index_t(max_hole_edges)
            );
        }

        if(max_degree3_dist > 0.0) {
            max_degree3_dist *= (0.01 * bbox_diagonal);
            GEO::remove_degree3_vertices(*mesh_grob(), max_degree3_dist);
        }
        
        if(remove_intersections) {
            Logger::out("Mesh") << "Removing intersections" << std::endl;
            GEO::mesh_remove_intersections(*mesh_grob());
            Logger::out("Mesh") << "Removed intersections" << std::endl;
        }
        
        mesh_grob()->update();
    }

    void MeshGrobSurfaceCommands::fill_holes(index_t max_nb_vertices) {
	GEO::mesh_repair(
	    *mesh_grob(),
	    GEO::MESH_REPAIR_TOPOLOGY,
	    0.0
	);
	GEO::fill_holes(*mesh_grob(), 1e30, max_nb_vertices, false);
	GEO::mesh_repair(
	    *mesh_grob(),
	    GEO::MeshRepairMode(
		GEO::MESH_REPAIR_COLOCATE | GEO::MESH_REPAIR_DUP_F
	    ),
	    0.0
	);
	GEO::mesh_repair(
	    *mesh_grob(),
	    GEO::MESH_REPAIR_TOPOLOGY,
	    0.0
	);
	mesh_grob()->update();
    }
    
    void MeshGrobSurfaceCommands::expand_border(double margin) {
        margin *= (GEO::bbox_diagonal(*mesh_grob()) * 0.001);
        GEO::expand_border(*mesh_grob(),margin);
        mesh_grob()->update();
    }


    void MeshGrobSurfaceCommands::fix_facets_orientation() {
        Attribute<double> attribute;
        attribute.bind_if_is_defined(mesh_grob()->facets.attributes(), "visibility");
        if(!attribute.is_bound()) {
            Logger::err("Attributes") << "visibility" << ": no such facet attribute of type double"
                                      << std::endl;
            Logger::err("Attributes")
                << "use Attributes->Facets->compute facets visibility"
                << std::endl;
            return;
        }
        for(index_t f: mesh_grob()->facets) {
            if(attribute[f] < 0.0) {
                mesh_grob()->facets.flip(f);
                attribute[f] = -attribute[f];
            }
        }
        mesh_grob()->facets.connect();
        mesh_grob()->update();
    }

    
    void MeshGrobSurfaceCommands::remove_invisible_facets(
        double min_visibility
    ) {
        Attribute<double> attribute;
        attribute.bind_if_is_defined(
            mesh_grob()->facets.attributes(), "visibility"
        );
        if(!attribute.is_bound()) {
            Logger::err("Attributes")
                << "visibility" << ": no such facet attribute of type double"
                << std::endl;
            Logger::err("Attributes")
                << "use Attributes->Facets->compute facets visibility"
                << std::endl;
            return;
        }
        vector<index_t> to_kill(mesh_grob()->facets.nb(),0);
        for(index_t f: mesh_grob()->facets) {
            to_kill[f] = (::fabs(attribute[f]) > min_visibility) ? 0 : 1;
        }
        mesh_grob()->facets.delete_elements(to_kill);
        mesh_grob()->update();
    }

    void MeshGrobSurfaceCommands::compute_union(
	const MeshGrobName& other_name,
	const NewMeshGrobName& result_name,
	bool pre_process,
	bool post_process
    ) {
	MeshGrob* other = MeshGrob::find(scene_graph(), other_name);
	if(other == nullptr) {
	    Logger::err("Booleans") << other_name << ": no such MeshGrob"
				    << std::endl;
	    return;
	}
	if(other == mesh_grob()) {
	    Logger::err("Booleans") << "Mesh and operand are the same"
				    << std::endl;
	    return;
	}
	if(!mesh_grob()->facets.are_simplices()) {
	    Logger::err("Booleans") << "Mesh is not triangulated" << std::endl;
	    return;
	}
	if(!other->facets.are_simplices()) {
	    Logger::err("Booleans") << other_name << " is not triangulated"
				    << std::endl;
	    return;	    
	}
	MeshGrob* result = MeshGrob::find_or_create(scene_graph(), result_name);
	if(pre_process) {
	    fix_mesh_for_boolean_ops(mesh_grob());
	    mesh_grob()->update();
	    fix_mesh_for_boolean_ops(other);
	    other->update();
	}
	mesh_union(*result, *mesh_grob(), *other);
	if(post_process) {
	    fix_mesh_for_boolean_ops(result);
	}
	result->update();
    }
    
    void MeshGrobSurfaceCommands::compute_intersection(
	const MeshGrobName& other_name,
	const NewMeshGrobName& result_name,
	bool pre_process,
	bool post_process
    ) {
	MeshGrob* other = MeshGrob::find(scene_graph(), other_name);
	if(other == nullptr) {
	    Logger::err("Booleans") << other_name << ": no such MeshGrob"
				    << std::endl;
	    return;
	}
	if(other == mesh_grob()) {
	    Logger::err("Booleans") << "Mesh and operand are the same"
				    << std::endl;
	    return;
	}
	if(!mesh_grob()->facets.are_simplices()) {
	    Logger::err("Booleans") << "Mesh is not triangulated" << std::endl;
	    return;
	}
	if(!other->facets.are_simplices()) {
	    Logger::err("Booleans") << other_name << " is not triangulated"
				    << std::endl;
	    return;	    
	}
	MeshGrob* result = MeshGrob::find_or_create(scene_graph(), result_name);
	if(pre_process) {
	    fix_mesh_for_boolean_ops(mesh_grob());
	    mesh_grob()->update();
	    fix_mesh_for_boolean_ops(other);
	    other->update();
	}
	mesh_intersection(*result, *mesh_grob(), *other);
	if(post_process) {
	    fix_mesh_for_boolean_ops(result);
	}
	result->update();
    }

    void MeshGrobSurfaceCommands::compute_difference(
	const MeshGrobName& other_name,
	const NewMeshGrobName& result_name,
	bool pre_process,
	bool post_process
    ) {
	MeshGrob* other = MeshGrob::find(scene_graph(), other_name);
	if(other == nullptr) {
	    Logger::err("Booleans") << other_name << ": no such MeshGrob"
				    << std::endl;
	    return;
	}
	if(other == mesh_grob()) {
	    Logger::err("Booleans") << "Mesh and operand are the same"
				    << std::endl;
	    return;
	}
	if(!mesh_grob()->facets.are_simplices()) {
	    Logger::err("Booleans") << "Mesh is not triangulated" << std::endl;
	    return;
	}
	if(!other->facets.are_simplices()) {
	    Logger::err("Booleans") << other_name << " is not triangulated"
				    << std::endl;
	    return;	    
	}
	MeshGrob* result = MeshGrob::find_or_create(scene_graph(), result_name);
	if(pre_process) {
	    fix_mesh_for_boolean_ops(mesh_grob());
	    mesh_grob()->update();
	    fix_mesh_for_boolean_ops(other);
	    other->update();
	}
	mesh_difference(*result, *mesh_grob(), *other);
	if(post_process) {
	    fix_mesh_for_boolean_ops(result);
	}
	result->update();
    }
    
    void MeshGrobSurfaceCommands::remesh_smooth(
        const NewMeshGrobName& surface_name_in,
        unsigned int nb_points,
        double tri_shape_adaptation,
        double tri_size_adaptation,
        unsigned int nb_normal_iter,
        unsigned int nb_Lloyd_iter,
        unsigned int nb_Newton_iter,
        unsigned int Newton_m,
        unsigned int LFS_samples
    ) {
        std::string surface_name = surface_name_in;
        
        if(surface_name == mesh_grob()->name()) {
            Logger::err("Remesh")
                << "remesh should not be the same as mesh"
                << std::endl;
            return;
        }

        if(mesh_grob()->facets.nb() == 0) {
            Logger::err("Remesh")
                << "mesh has no facet" << std::endl;
            return;
        }
        
        if(!mesh_grob()->facets.are_simplices()) {
            Logger::err("Remesh")
                << "mesh need to be simplicial, use repair"
                << std::endl;
            return;
        }
       
        index_t dimension = mesh_grob()->vertices.dimension();
       
        MeshGrob* remesh = MeshGrob::find_or_create(
            scene_graph(), surface_name
        );

        remesh->clear();
        remesh->lock_graphics();        

        if(tri_shape_adaptation != 0.0) {
            tri_shape_adaptation *= 0.02;
            GEO::compute_normals(*mesh_grob());
            mesh_grob()->update();
            if(nb_normal_iter != 0) {
                GEO::Logger::out("Nsmooth") << "Smoothing normals, "
                                            << nb_normal_iter
                                            << " iteration(s)" << std::endl;
                GEO::simple_Laplacian_smooth(
                    *mesh_grob(), nb_normal_iter, true
                );
            }
            GEO::set_anisotropy(*mesh_grob(), tri_shape_adaptation);
            mesh_grob()->update();
        } else {
            mesh_grob()->vertices.set_dimension(3);
            mesh_grob()->update();            
        }

        if(tri_size_adaptation != 0.0) {
            GEO::compute_sizing_field(
                *mesh_grob(), tri_size_adaptation, LFS_samples
            );
            mesh_grob()->update();            
        } else {
            AttributesManager& attributes = mesh_grob()->vertices.attributes();
            if(attributes.is_defined("weight")) {
                attributes.delete_attribute_store("weight");
                mesh_grob()->update();                
            }
        }
        
        GEO::remesh_smooth(
            *mesh_grob(), *remesh,
            nb_points, 0,
            nb_Lloyd_iter, nb_Newton_iter, Newton_m
        );
        
        if(remesh->get_shader() != nullptr) {
            remesh->get_shader()->set_property("mesh_style", "true;0 0 0 1;1");
        }

        remesh->unlock_graphics();
        remesh->update();
       
        // If anisotropic remeshing was used, then this lifts
        // the mesh into 6D space. Here we restore the initial
        // 3D setting.
        if(mesh_grob()->vertices.dimension() != dimension) {
           mesh_grob()->vertices.set_dimension(dimension);
        }
       
        // Need to update this one as well, since vertices
        //  order may have changed
        mesh_grob()->update(); 
    }       

    void MeshGrobSurfaceCommands::remesh_feature_sensitive(
        const NewMeshGrobName& surface_name,
        unsigned int nb_points,
        bool refine,
        double max_dist,
        double normal_anisotropy,
        unsigned int nb_Lloyd_iter,
        unsigned int nb_Newton_iter,
        unsigned int nb_LpCVT_iter,
        unsigned int Newton_m,
        bool RVC_centroids
    ) {
        if(surface_name == mesh_grob()->name()) {
            Logger::err("Remesh")
                << "remesh should not be the same as mesh"
                << std::endl;
            return;
        }
        
        if(mesh_grob()->facets.nb() == 0) {
            Logger::err("Remesh")
                << "mesh has no facet" << std::endl;
            return;
        }
        
        if(!mesh_grob()->facets.are_simplices()) {
            Logger::err("Remesh")
                << "mesh need to be simplicial, use repair"
                << std::endl;
            return;
        }
#ifdef GEOGRAM_WITH_VORPALINE        
        MeshGrob* remesh = MeshGrob::find_or_create(
            scene_graph(), surface_name
        );
        remesh->clear();
        bool RVC_centroids_bkp =
            GEO::CmdLine::get_arg_bool("remesh:RVC_centroids");
        GEO::CmdLine::set_arg("remesh:RVC_centroids",RVC_centroids);
        GEO::remesh_feature_sensitive(
            *mesh_grob(), *remesh,
            nb_points,
            refine, max_dist,
            normal_anisotropy,
            nb_Lloyd_iter, nb_Newton_iter, nb_LpCVT_iter, Newton_m
        );
        GEO::CmdLine::set_arg("remesh:RVC_centroids",RVC_centroids_bkp);        
        remesh->update();
        // Need to update this one as well,
        // since vertices order may have changed        
        mesh_grob()->update(); 
#else
        geo_argused(surface_name);
        geo_argused(nb_points);
        geo_argused(normal_anisotropy);
        geo_argused(nb_Lloyd_iter);
        geo_argused(nb_Newton_iter);
        geo_argused(nb_LpCVT_iter);
        geo_argused(Newton_m);
        geo_argused(RVC_centroids);
        geo_argused(refine);
        geo_argused(max_dist);
        Logger::err("Remesh") << "Needs Vorpaline/Vorpalib, contact authors"
                              << std::endl;
#endif
    }

   /***********************************************************/

    void MeshGrobSurfaceCommands::remesh_quad_dominant(
	const NewMeshGrobName& surface_name,
	double rel_edge_len,
	bool sharp_features,
	bool optimize_parity,
	double max_scaling_corr
    ) {
        if(surface_name == mesh_grob()->name()) {
            Logger::err("Remesh")
                << "remesh should not be the same as mesh"
                << std::endl;
            return;
        }
        
        if(mesh_grob()->facets.nb() == 0) {
            Logger::err("Remesh")
                << "mesh has no facet" << std::endl;
            return;
        }
        
        if(!mesh_grob()->facets.are_simplices()) {
            Logger::err("Remesh")
                << "mesh need to be simplicial, use repair"
                << std::endl;
            return;
        }
#ifdef GEOGRAM_WITH_VORPALINE
        MeshGrob* remesh = MeshGrob::find_or_create(
            scene_graph(), surface_name
        );
        remesh->clear();

	mesh_quad_dominant(
	    *mesh_grob(), *remesh,
	    rel_edge_len,
	    sharp_features,
	    optimize_parity,
	    max_scaling_corr
	);
	
        remesh->update();
        // Need to update this one as well,
        // since vertices order may have changed        
        mesh_grob()->update(); 
#else
	geo_argused(surface_name);
	geo_argused(rel_edge_len);
	geo_argused(optimize_parity);
	geo_argused(max_scaling_corr);
        Logger::err("Remesh") << "Needs Vorpaline/Vorpalib, contact authors"
                              << std::endl;
#endif	
    }

   /***********************************************************/
    
    void MeshGrobSurfaceCommands::decimate(
        index_t nb_bins,
        bool remove_deg3_vrtx,
        bool keep_borders,
        bool repair
    ) {
        MeshDecimateMode mode = MESH_DECIMATE_DUP_F;
        if(remove_deg3_vrtx) {
            mode = MeshDecimateMode(mode | MESH_DECIMATE_DEG_3);
        }
        if(keep_borders) {
            mode = MeshDecimateMode(mode | MESH_DECIMATE_KEEP_B);
        }
        mesh_decimate_vertex_clustering(*mesh_grob(), nb_bins, mode);
        if(repair) {
            repair_surface();
        }
        mesh_grob()->update();
    }

   /***********************************************************/
    
    void MeshGrobSurfaceCommands::project_on_surface(
	const MeshGrobName& surface_name
    ) {
        MeshGrob* surface = MeshGrob::find(scene_graph(), surface_name);
        if(surface == nullptr) {
            Logger::err("Mesh")
		<< surface_name << ": no such surface" << std::endl;
            return;
        }
        
        //   We need to lock the graphics because the AABB will change
        // the order of the surface facets.
        surface->lock_graphics();
        MeshFacetsAABB AABB(*surface);

        for(index_t i: mesh_grob()->vertices) {
            vec3 p(mesh_grob()->vertices.point_ptr(i));
            vec3 q;
            double sq_dist;
            AABB.nearest_facet(p,q,sq_dist);
            for(index_t c=0; c<3; ++c) {
                mesh_grob()->vertices.point_ptr(i)[c] = q[c];
            }
        }
        
        surface->unlock_graphics();
        mesh_grob()->update();
    }

    void MeshGrobSurfaceCommands::split_triangles(index_t nb_times) {
	if(!mesh_grob()->facets.are_simplices()) {
	    Logger::err("Split") << "Mesh is not simplicial, cannot split."
				 << std::endl;
	    return;
	}
	for(index_t i=0; i<nb_times; ++i) {
	    mesh_split_triangles(*mesh_grob());
	}
	mesh_grob()->update();
    }

    void MeshGrobSurfaceCommands::split_quads(index_t nb_times) {
	for(index_t i=0; i<nb_times; ++i) {
	    mesh_split_quads(*mesh_grob());
	}
	mesh_grob()->update();
    }

    void MeshGrobSurfaceCommands::split_catmull_clark(index_t nb_times) {
	for(index_t i=0; i<nb_times; ++i) {
	    mesh_split_catmull_clark(*mesh_grob());
	}
	mesh_grob()->update();
    }
    
    void MeshGrobSurfaceCommands::tessellate_facets(
	index_t max_vertices_per_facet
    ) {
	GEO::tessellate_facets(*mesh_grob(),max_vertices_per_facet);
        if(mesh_grob()->get_shader() != nullptr) {
            mesh_grob()->get_shader()->set_property(
		"mesh_style", "true;0 0 0 1;1"
	    );
        }
	mesh_grob()->update();
    }
    
   
    void MeshGrobSurfaceCommands::triangulate_center_vertex() {
	GEO::mesh_triangulate_center_vertex(*mesh_grob());
        if(mesh_grob()->get_shader() != nullptr) {
            mesh_grob()->get_shader()->set_property(
		"mesh_style", "true;0 0 0 1;1"
	    );
        }
	mesh_grob()->update();
    }

   
    void MeshGrobSurfaceCommands::smooth() {
	Attribute<bool> v_is_locked(
	    mesh_grob()->vertices.attributes(),"selection"
	);
	bool has_locked_v = false;
	for(index_t v: mesh_grob()->vertices) {
	    if(v_is_locked[v]) {
		has_locked_v = true;
		break;
	    }
	}
	if(!has_locked_v) {
	    Logger::err("Smooth") << "Mesh has no locked vertex"
				  << std::endl;
	    return;
	}
	mesh_smooth(*mesh_grob());
	mesh_grob()->update();	
    }

    void MeshGrobSurfaceCommands::make_texture_atlas(
	bool unglue_sharp_edges,
	double sharp_angles_threshold,
	ChartParameterizer param,
	ChartPacker pack,
	bool verbose
    ) {
	if(!unglue_sharp_edges) {
	    sharp_angles_threshold = 360.0;
	}
	mesh_make_atlas(
	    *mesh_grob(),
	    sharp_angles_threshold * M_PI / 180.0,
	    param,
	    pack,
	    verbose
	);
	mesh_grob()->update();
    }


    void MeshGrobSurfaceCommands::pack_texture_space(
	ChartPacker pack
    ) {
	Packer packer;
	packer.pack_surface(*mesh_grob(), false);
	if(pack == PACK_XATLAS) {
	    pack_atlas_using_xatlas(*mesh_grob());
	}
	mesh_grob()->update();
    }
    
    void MeshGrobSurfaceCommands::parameterize_chart(
	const std::string& attribute, ChartParameterizer algo, bool verbose
    ) {
	switch(algo) {
	    case PARAM_LSCM:
		mesh_compute_LSCM(*mesh_grob(), attribute, false, "", verbose);
		break;
	    case PARAM_SPECTRAL_LSCM:
		mesh_compute_LSCM(*mesh_grob(), attribute, true, "", verbose);
		break;
	    case PARAM_ABF:
		if(!mesh_grob()->facets.are_simplices()) {
		    Logger::err("ABF")
		      << "Mesh facets need to be triangles" << std::endl;
		    return ;
		} 
		mesh_compute_ABF_plus_plus(*mesh_grob(), attribute, verbose);
		break;
	}
	mesh_grob()->update();		
    }

    void MeshGrobSurfaceCommands::bake_normals(
	const MeshGrobName& surface,
	index_t size,
	const NewImageFileName& image,
	index_t nb_dilate,
	const std::string& attribute
    ) {
	Attribute<double> tex_coord;
	tex_coord.bind_if_is_defined(
	    mesh_grob()->facet_corners.attributes(), attribute
	);
	if(!tex_coord.is_bound()) {
	    Logger::err("baking")
		<< attribute << ": no such facet corner attribute" << std::endl;
	    return;
	}
	if(tex_coord.dimension() != 2) {
	    Logger::err("baking")
		<< attribute << ": wrong dimension" << std::endl;
	    return;
	}

	Image_var normal_map = new Image(Image::RGB, Image::BYTE, size, size);
	
	MeshGrob* highres = MeshGrob::find(scene_graph(),surface);
	
	if(highres == mesh_grob()) {
	    
	    // Case 1: bake normals from the parameterized surface.
	    
	    bake_mesh_facet_normals(mesh_grob(), normal_map);
	    
	} else {

	    // Case 2: bake normals from a different highres surface.

	    //   Step 1: create a "geometry image" from the parameterized
	    // mesh.

	    Logger::out("baking") << "Creating geometry image"
				  << std::endl;
	    
	    Image_var geometry_image = new Image(
		Image::RGB, Image::FLOAT64, size, size
	    );

	    bake_mesh_geometry(mesh_grob(),geometry_image);
	    
	    //   Step 2: create the normal map by looking up the high res
	    // facet nearest to each point from the geometry image.

	    if(highres->facets.nb() != 0) {
	    
		Logger::out("baking")
		    << "Transferring highres surface normals to geometry image"
		    << std::endl;
		
		bake_mesh_facet_normals_indirect(
		    geometry_image, normal_map, highres
		);
		
	    } else {

		Logger::out("baking")
		    << "Transferring highres pointset normals to geometry image"
		    << std::endl;

		Attribute<double> normal;
		normal.bind_if_is_defined(
		    highres->vertices.attributes(), "normal"
		);
		if(!normal.is_bound()) {
		    Logger::out("baking")
			<< "\'normal\': no such vertex attribute"
			<< std::endl;
		    return;
		}

		bake_mesh_points_attribute_indirect(
		    geometry_image, normal_map, highres, normal,
		    1.0, 0.5 // scale and bias to map [-1,1] to [0,1]
		);
	    }
	}

	MorphoMath mm(normal_map);
	mm.dilate(nb_dilate);
	ImageLibrary::instance()->save_image(image, normal_map);

	Object* shader = mesh_grob()->get_shader();
	if(shader != nullptr) {
	    shader->set_property("painting","TEXTURE");
	    shader->set_property("tex_image",image);
	    shader->set_property("tex_coords","facet_corners." + attribute);
	    shader->set_property("normal_map","true");
	}

	mesh_grob()->update();
	
    }

    void MeshGrobSurfaceCommands::bake_colors(
	const MeshGrobName& surface,
	const std::string& color_attr_name,
	index_t size,
	const NewImageFileName& image,
	index_t nb_dilate,
	const std::string& attribute
    ) {
	Attribute<double> tex_coord;
	tex_coord.bind_if_is_defined(
	    mesh_grob()->facet_corners.attributes(), attribute
	);
	if(!tex_coord.is_bound()) {
	    Logger::err("baking")
		<< attribute << ": no such facet corner attribute" << std::endl;
	    return;
	}
	if(tex_coord.dimension() != 2) {
	    Logger::err("baking")
		<< attribute << ": wrong dimension" << std::endl;
	    return;
	}

	MeshGrob* highres = MeshGrob::find(scene_graph(),surface);
	if(highres == nullptr) {
	    Logger::err("baking") << surface << ": no such MeshGrob"
				  << std::endl;
	    return;
	}
	
	Attribute<double> color;
	color.bind_if_is_defined(
	   highres->vertices.attributes(), color_attr_name
	);
	if(!color.is_bound()) {
	    Logger::err("baking") << color_attr_name 
	  	  		  << ": no such vertex attribute"
				  << std::endl;
	    return;
	}
	if(color.dimension() < 3) {
	    Logger::err("baking") << color_attr_name
				  << ": wront dimension" << std::endl;
	    return;
	}
	
	Image_var color_map = new Image(Image::RGB, Image::BYTE, size, size);
	
	if(highres == mesh_grob()) {
	    
	    // Case 1: bake colors from the parameterized surface.
	    
	    bake_mesh_attribute(mesh_grob(), color_map, color);
	    
	} else {

	    // Case 2: bake normals from a different highres surface.

	    //   Step 1: create a "geometry image" from the parameterized
	    // mesh.

	    Logger::out("baking") << "Creating geometry image"
				  << std::endl;
	    
	    Image_var geometry_image = new Image(
		Image::RGB, Image::FLOAT64, size, size
	    );

	    bake_mesh_geometry(mesh_grob(),geometry_image);
	    
	    //   Step 2: create the color map by looking up the high res
	    // point nearest to each point from the geometry image.

	    bake_mesh_points_attribute_indirect(
		 geometry_image, color_map, highres, color
	    );
	}

	MorphoMath mm(color_map);
	mm.dilate(nb_dilate);
	ImageLibrary::instance()->save_image(image, color_map);

	Object* shader = mesh_grob()->get_shader();
	if(shader != nullptr) {
	    shader->set_property("painting","TEXTURE");
	    shader->set_property("tex_image",image);
	    shader->set_property("tex_coords","facet_corners." + attribute);
	    shader->set_property("normal_map","false");
	}

	mesh_grob()->update();
    }

    void MeshGrobSurfaceCommands::project_vertices_on_surface(
	const MeshGrobName& surface_name
    ) {
	MeshGrob* surface = MeshGrob::find(scene_graph(), surface_name);
	if(surface == nullptr) {
	    Logger::out("Surface") << surface_name << ": no such MeshGrob"
				   << std::endl;
	    return;
	}

	if(surface == mesh_grob()) {
	    Logger::out("Surface") << "Cannot project surface onto itself"
				   << std::endl;
	}
	
	if(surface->facets.nb() == 0) {
	    Logger::out("Surface") << surface_name << " has no facets"
				   << std::endl;
	    return;
	}

	MeshFacetsAABB AABB(*surface);

	for(index_t i=0; i<mesh_grob()->vertices.nb(); ++i) {
	    vec3 p(mesh_grob()->vertices.point_ptr(i));
	    vec3 q;
	    double sq_dist;
	    AABB.nearest_facet(p,q,sq_dist);
	    mesh_grob()->vertices.point_ptr(i)[0] = q.x;
	    mesh_grob()->vertices.point_ptr(i)[1] = q.y;
	    mesh_grob()->vertices.point_ptr(i)[2] = q.z;		
	}

	mesh_grob()->update();
    }

    
}

