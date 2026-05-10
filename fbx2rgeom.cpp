#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define UFBX_NO_EXCEPTIONS
#define UFBX_REAL_IS_FLOAT
#include "ufbx.h"

#include "export.h"
extern "C" {
#include "geombuf.h"
}
#include "rgeom.h"
#include "vector.h"

namespace fs = std::filesystem;

namespace {

struct MaterialInfo
{
	std::string name;
	std::string texture_path;
	bool two_uvs = false;
};

struct TriangleBuild
{
	st_RawTriangle tri {};
	bool has_normal[3] = { false, false, false };
};

struct FbxData
{
	std::vector<std::array<float, 3>> positions;
	std::vector<std::array<float, 3>> normals;
	std::vector<std::array<float, 2>> texcoords0;
	std::vector<std::array<float, 2>> texcoords1;
	std::vector<TriangleBuild> triangles;
	std::vector<MaterialInfo> materials;
	std::unordered_map<std::string, int> used_materials;
	int skipped_degenerate_tris = 0;
};

static std::string trim(const std::string &s)
{
	size_t start = 0;
	while(start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
		start++;

	size_t end = s.size();
	while(end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
		end--;

	return s.substr(start, end - start);
}

static bool starts_with(const std::string &value, const char *prefix)
{
	size_t prefix_len = std::strlen(prefix);
	return value.size() >= prefix_len && value.compare(0, prefix_len, prefix) == 0;
}

static int ensure_material(FbxData &fbx, const std::string &name, const std::string &texture_path, bool two_uvs)
{
	auto it = fbx.used_materials.find(name);
	if(it != fbx.used_materials.end())
		return it->second;

	MaterialInfo material;
	material.name = name;
	material.texture_path = texture_path;
	material.two_uvs = two_uvs;

	int index = static_cast<int>(fbx.materials.size());
	fbx.materials.push_back(std::move(material));
	fbx.used_materials[name] = index;
	return index;
}

static bool load_fbx_file(FbxData &fbx, const fs::path &fbx_path, std::string &error)
{
	ufbx_load_opts opts = {};
	opts.target_axes = ufbx_axes_right_handed_y_up;
	opts.target_unit_meters = 1.0f;
	opts.generate_missing_normals = true;

	ufbx_error ufbx_error;
	ufbx_scene *scene = ufbx_load_file(fbx_path.string().c_str(), &opts, &ufbx_error);

	if(!scene) {
		char buf[512];
		ufbx_format_error(buf, sizeof(buf), &ufbx_error);
		error = std::string("FBX load failed: ") + buf;
		return false;
	}

	for(size_t mesh_idx = 0; mesh_idx < scene->meshes.count; mesh_idx++) {
		ufbx_mesh *mesh = scene->meshes[mesh_idx];

		if(mesh->num_triangles == 0)
			continue;

		int base_position = static_cast<int>(fbx.positions.size());

		for(size_t i = 0; i < mesh->num_vertices; i++) {
			const ufbx_vec3 &p = mesh->vertices[i];
			fbx.positions.push_back({ static_cast<float>(p.x), static_cast<float>(p.z), -static_cast<float>(p.y) });
		}

		bool has_uv0 = mesh->uv_sets.count > 0 && mesh->uv_sets[0].vertex_uv.exists;
		bool has_uv1 = mesh->uv_sets.count > 1 && mesh->uv_sets[1].vertex_uv.exists;

		for(size_t face_idx = 0; face_idx < mesh->num_faces; face_idx++) {
			const ufbx_face &face = mesh->faces[face_idx];

			if(face.num_indices < 3)
				continue;

			int material_id = -1;
			std::string material_name = "default";
			std::string texture_path;
			bool two_uvs = false;

			if(mesh->face_material.data && face_idx < mesh->face_material.count) {
				uint32_t face_mat_idx = mesh->face_material[face_idx];
				if(face_mat_idx < mesh->materials.count) {
					ufbx_material *mat = mesh->materials[face_mat_idx];
					if(mat) {
						material_name = mat->name.data ? mat->name.data : "default";

						two_uvs = starts_with(material_name, "2T");

						if(mat->fbx.diffuse_color.texture && mat->fbx.diffuse_color.texture->filename.data) {
							texture_path = mat->fbx.diffuse_color.texture->filename.data;
						} else if(mat->fbx.diffuse_color.texture && mat->fbx.diffuse_color.texture->relative_filename.data) {
							texture_path = mat->fbx.diffuse_color.texture->relative_filename.data;
						}

						if(texture_path.empty() && mat->textures.count > 0) {
							ufbx_texture *tex = mat->textures[0].texture;
							if(tex) {
								if(tex->relative_filename.data)
									texture_path = tex->relative_filename.data;
								else if(tex->filename.data)
									texture_path = tex->filename.data;
							}
						}
					}
				}
			}

			material_id = ensure_material(fbx, material_name, texture_path, two_uvs);

			for(size_t tri = 0; tri + 2 < face.num_indices; tri++) {
				TriangleBuild triangle {};
				size_t mesh_indices[3] = {
					face.index_begin + 0,
					face.index_begin + tri + 1,
					face.index_begin + tri + 2,
				};

				for(int v = 0; v < 3; v++) {
					size_t mesh_index = mesh_indices[v];
					uint32_t idx = mesh->vertex_indices[mesh_index];

					triangle.tri.v[v].point = base_position + static_cast<int>(idx);

					vset4(triangle.tri.v[v].color_rgba, 1.0f, 1.0f, 1.0f, 1.0f);

					if(has_uv0) {
						const ufbx_uv_set &uv_set = mesh->uv_sets[0];
						ufbx_vec2 uv = ufbx_get_vertex_vec2(&uv_set.vertex_uv, mesh_index);
						triangle.tri.v[v].uv[0][0] = static_cast<float>(uv.x);
						triangle.tri.v[v].uv[0][1] = 1.0f - static_cast<float>(uv.y);
					}

					if(has_uv1 && two_uvs) {
						const ufbx_uv_set &uv_set = mesh->uv_sets[1];
						ufbx_vec2 uv = ufbx_get_vertex_vec2(&uv_set.vertex_uv, mesh_index);
						triangle.tri.v[v].uv[1][0] = static_cast<float>(uv.x);
						triangle.tri.v[v].uv[1][1] = 1.0f - static_cast<float>(uv.y);
					}

					if(mesh->vertex_normal.exists) {
						ufbx_vec3 n = ufbx_get_vertex_vec3(&mesh->vertex_normal, mesh_index);
						triangle.tri.v[v].normal[0] = static_cast<float>(n.x);
						triangle.tri.v[v].normal[1] = static_cast<float>(n.z);
						triangle.tri.v[v].normal[2] = -static_cast<float>(n.y);
						triangle.has_normal[v] = true;
					} else {
						vset3(triangle.tri.v[v].normal, 0.0f, 0.0f, 0.0f);
					}
				}

				triangle.tri.surface_tag = reinterpret_cast<const void *>(static_cast<intptr_t>(material_id));

				float edge1[3];
				float edge2[3];
				float cross[3];
				vcopy(edge1, fbx.positions[triangle.tri.v[1].point].data());
				vsub(edge1, fbx.positions[triangle.tri.v[0].point].data());
				vcopy(edge2, fbx.positions[triangle.tri.v[2].point].data());
				vsub(edge2, fbx.positions[triangle.tri.v[0].point].data());
				vcross(cross, edge1, edge2);
				float area2 = vlen(cross);
				if(area2 <= 1e-7f) {
					fbx.skipped_degenerate_tris++;
					continue;
				}

				fbx.triangles.push_back(triangle);
			}
		}
	}

	ufbx_free_scene(scene);

	if(fbx.positions.empty()) {
		error = "FBX contains no vertices";
		return false;
	}

	if(fbx.triangles.empty()) {
		error = "FBX contains no triangles";
		return false;
	}

	return true;
}

static void fill_missing_normals(FbxData &fbx)
{
	std::vector<std::array<float, 3>> face_normals(fbx.triangles.size());

	for(size_t i = 0; i < fbx.triangles.size(); i++) {
		const TriangleBuild &triangle = fbx.triangles[i];
		float normal[3];
		vtrinormal(
			normal,
			fbx.positions[triangle.tri.v[0].point].data(),
			fbx.positions[triangle.tri.v[1].point].data(),
			fbx.positions[triangle.tri.v[2].point].data());
		face_normals[i] = { normal[0], normal[1], normal[2] };

		for(int j = 0; j < 3; j++) {
			if(!triangle.has_normal[j]) {
				fbx.triangles[i].tri.v[j].normal[0] = normal[0];
				fbx.triangles[i].tri.v[j].normal[1] = normal[1];
				fbx.triangles[i].tri.v[j].normal[2] = normal[2];
			}
		}
	}
}

static void free_packed_parts(std::vector<st_RGeomPart> &parts)
{
	for(st_RGeomPart &part : parts) {
		std::free(part.geom.verts);
		std::free(part.geom.indices);
		part.geom.verts = nullptr;
		part.geom.indices = nullptr;
	}
}

static void free_raw_model(st_RawModel &raw_model)
{
	std::free(raw_model.points);
	std::free(raw_model.tris);
	raw_model.points = nullptr;
	raw_model.tris = nullptr;
}

static bool build_export_data(
	const FbxData &fbx,
	st_RawModel &raw_model,
	st_RGeomMtlList &mtl_list,
	std::vector<st_RGeomPart> &parts,
	std::vector<st_RGeomMtlList::st_RGeomMtl> &mtl_storage,
	std::string &error)
{
	std::memset(&raw_model, 0, sizeof(raw_model));
	std::memset(&mtl_list, 0, sizeof(mtl_list));

	raw_model.points_count = static_cast<int>(fbx.positions.size());
	raw_model.points = static_cast<float(*)[3]>(std::malloc(sizeof(float[3]) * fbx.positions.size()));
	raw_model.tris_count = static_cast<int>(fbx.triangles.size());
	raw_model.tris = static_cast<st_RawTriangle *>(std::calloc(fbx.triangles.size(), sizeof(st_RawTriangle)));

	if(!raw_model.points || !raw_model.tris) {
		error = "out of memory while building export buffers";
		return false;
	}

	for(size_t i = 0; i < fbx.positions.size(); i++) {
		raw_model.points[i][0] = fbx.positions[i][0];
		raw_model.points[i][1] = fbx.positions[i][1];
		raw_model.points[i][2] = fbx.positions[i][2];
	}

	for(size_t i = 0; i < fbx.triangles.size(); i++)
		raw_model.tris[i] = fbx.triangles[i].tri;

	std::vector<int> material_remap(fbx.materials.size(), -1);
	std::vector<int> used_material_ids;
	used_material_ids.reserve(fbx.materials.size());
	for(const TriangleBuild &triangle : fbx.triangles) {
		int old_material_id = static_cast<int>(reinterpret_cast<intptr_t>(triangle.tri.surface_tag));
		if(old_material_id < 0 || old_material_id >= static_cast<int>(fbx.materials.size())) {
			error = "internal error: triangle material id out of range";
			return false;
		}

		if(material_remap[old_material_id] < 0) {
			material_remap[old_material_id] = static_cast<int>(used_material_ids.size());
			used_material_ids.push_back(old_material_id);
		}
	}

	parts.assign(used_material_ids.size(), {});
	std::vector<st_GeomBuf> geom_bufs(used_material_ids.size());
	for(size_t i = 0; i < used_material_ids.size(); i++)
		GeomBuf_Init(&geom_bufs[i], sizeof(st_RawVertex), 65535);

	for(const TriangleBuild &triangle : fbx.triangles) {
		int old_material_id = static_cast<int>(reinterpret_cast<intptr_t>(triangle.tri.surface_tag));
		if(old_material_id < 0 || old_material_id >= static_cast<int>(material_remap.size())) {
			error = "internal error: triangle material id out of range";
			for(st_GeomBuf &buf : geom_bufs)
				GeomBuf_Destroy(&buf);
			return false;
		}

		int material_id = material_remap[old_material_id];
		if(material_id < 0 || material_id >= static_cast<int>(geom_bufs.size())) {
			error = "internal error: remapped material id out of range";
			for(st_GeomBuf &buf : geom_bufs)
				GeomBuf_Destroy(&buf);
			return false;
		}

		int add_result = GeomBuf_AddTri(
			&geom_bufs[material_id],
			const_cast<st_RawVertex *>(&triangle.tri.v[0]),
			const_cast<st_RawVertex *>(&triangle.tri.v[1]),
			const_cast<st_RawVertex *>(&triangle.tri.v[2]));

		if(add_result == ERROR_BUFFERISFULL) {
			error = "material '" + fbx.materials[old_material_id].name + "' exceeds 65535 unique vertices";
			for(st_GeomBuf &buf : geom_bufs)
				GeomBuf_Destroy(&buf);
			return false;
		}

		if(add_result == ERROR_NOMEM) {
			error = "out of memory while packing geometry";
			for(st_GeomBuf &buf : geom_bufs)
				GeomBuf_Destroy(&buf);
			return false;
		}
	}

	for(size_t i = 0; i < parts.size(); i++) {
		parts[i].material_id = static_cast<int>(i);
		parts[i].geom.verts_count = static_cast<int>(geom_bufs[i].verts_count);
		parts[i].geom.indices_count = static_cast<int>(geom_bufs[i].tris_count * 3);
		parts[i].geom.verts = reinterpret_cast<st_RawVertex *>(geom_bufs[i].verts);
		parts[i].geom.indices = geom_bufs[i].tris;
	}

	mtl_storage.resize(used_material_ids.size());
	for(size_t i = 0; i < used_material_ids.size(); i++) {
		const MaterialInfo &material = fbx.materials[used_material_ids[i]];
		mtl_storage[i].name = const_cast<char *>(material.name.c_str());
		mtl_storage[i].texture = material.texture_path.empty()
			? nullptr
			: const_cast<char *>(material.texture_path.c_str());
		mtl_storage[i].sm_angle = -1.0f;
		mtl_storage[i].sm_angle_cos = -1.0f;
	}

	mtl_list.count = static_cast<int>(mtl_storage.size());
	mtl_list.materials = mtl_storage.data();
	return true;
}

static fs::path make_output_path(const fs::path &input_path, const char *output_arg)
{
	fs::path default_name = input_path.stem();
	default_name += ".rgeom";

	if(output_arg == nullptr || output_arg[0] == '\0')
		return input_path.parent_path() / default_name;

	fs::path output_path(output_arg);
	if(fs::exists(output_path) && fs::is_directory(output_path))
		return output_path / default_name;

	return output_path;
}

static void print_usage()
{
	std::cerr << "Usage: fbx2rgeom <input.fbx> [output.rgeom]\n";
}

} // namespace

int main(int argc, char **argv)
{
	if(argc < 2 || argc > 3) {
		print_usage();
		return 1;
	}

	const fs::path input_path = fs::path(argv[1]);
	const fs::path output_path = make_output_path(input_path, argc >= 3 ? argv[2] : nullptr);

	FbxData fbx;
	std::string error;
	if(!load_fbx_file(fbx, input_path, error)) {
		std::cerr << "FBX load failed: " << error << "\n";
		return 1;
	}

	fill_missing_normals(fbx);

	st_RawModel raw_model {};
	st_RGeomMtlList mtl_list {};
	std::vector<st_RGeomPart> parts;
	std::vector<st_RGeomMtlList::st_RGeomMtl> mtl_storage;
	RCollision collision {};

	if(!build_export_data(fbx, raw_model, mtl_list, parts, mtl_storage, error)) {
		std::cerr << "Preparation failed: " << error << "\n";
		free_raw_model(raw_model);
		return 1;
	}

	int collision_result = create_collision(&collision, &raw_model);
	if(collision_result != COLL_OK) {
		free_packed_parts(parts);
		free_raw_model(raw_model);

		if(collision_result == COLL_OUTOFMEM) {
			std::cerr << "Collision generation failed: out of memory\n";
		} else if(collision_result == COLL_TOOMANYPOINTS) {
			std::cerr << "Collision generation failed: too many unique collision points (>65535)\n";
		} else {
			std::cerr << "Collision generation failed with code " << collision_result << "\n";
		}
		return 1;
	}

	int export_result = export_rgeom(
		output_path.string().c_str(),
		&mtl_list,
		static_cast<unsigned>(parts.size()),
		parts.data(),
		&raw_model,
		&collision,
		0);

	free_packed_parts(parts);
	free_collision(&collision);
	free_raw_model(raw_model);

	if(export_result != 0) {
		std::cerr << "Export failed with code " << export_result << "\n";
		return 1;
	}

	std::cout
		<< "Wrote " << output_path.string()
		<< " (" << raw_model.points_count << " points, "
		<< raw_model.tris_count << " triangles, "
		<< parts.size() << " material parts";
	if(fbx.skipped_degenerate_tris > 0)
		std::cout << ", skipped " << fbx.skipped_degenerate_tris << " degenerate triangles";
	std::cout << ")\n";

	return 0;
}
