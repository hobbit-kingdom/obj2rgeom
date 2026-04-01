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

#include "export.h"
#include "geombuf.h"
#include "rgeom.h"
#include "vector.h"

namespace fs = std::filesystem;

namespace {

struct MaterialInfo
{
	std::string name;
	std::string texture_path;
};

struct TriangleBuild
{
	st_RawTriangle tri {};
	bool has_normal[3] = { false, false, false };
	int smoothing_group = 0;
};

struct ObjData
{
	std::vector<std::array<float, 3>> positions;
	std::vector<std::array<float, 2>> texcoords;
	std::vector<std::array<float, 3>> normals;
	std::vector<TriangleBuild> triangles;
	std::vector<MaterialInfo> materials;
	std::unordered_map<std::string, int> used_materials;
	std::unordered_map<std::string, std::string> material_textures;
	int next_smoothing_group_id = 1;
	std::unordered_map<std::string, int> smoothing_groups;
	int skipped_degenerate_tris = 0;
};

struct FaceVertex
{
	int position = -1;
	int texcoord = -1;
	int normal = -1;
};

struct NormalKey
{
	int point = -1;
	int smoothing_group = 0;

	bool operator==(const NormalKey &other) const
	{
		return point == other.point && smoothing_group == other.smoothing_group;
	}
};

struct NormalKeyHash
{
	size_t operator()(const NormalKey &key) const
	{
		size_t h1 = std::hash<int>{}(key.point);
		size_t h2 = std::hash<int>{}(key.smoothing_group);
		return h1 ^ (h2 + 0x9e3779b9u + (h1 << 6) + (h1 >> 2));
	}
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

static std::vector<std::string> split_whitespace(const std::string &line)
{
	std::istringstream iss(line);
	std::vector<std::string> parts;
	std::string part;
	while(iss >> part)
		parts.push_back(part);
	return parts;
}

static bool try_parse_int(const std::string &text, int &value)
{
	char *end = nullptr;
	long parsed = std::strtol(text.c_str(), &end, 10);
	if(end == text.c_str() || *end != '\0')
		return false;
	value = static_cast<int>(parsed);
	return true;
}

static bool try_parse_float(const std::string &text, float &value)
{
	char *end = nullptr;
	float parsed = std::strtof(text.c_str(), &end);
	if(end == text.c_str() || *end != '\0')
		return false;
	value = parsed;
	return true;
}

static int resolve_obj_index(int raw_index, int count)
{
	if(raw_index > 0)
		return raw_index - 1;
	if(raw_index < 0)
		return count + raw_index;
	return -1;
}

static std::string get_rest_after_keyword(const std::string &line)
{
	size_t first_space = line.find_first_of(" \t");
	if(first_space == std::string::npos)
		return {};
	return trim(line.substr(first_space + 1));
}

static int ensure_smoothing_group(ObjData &obj, const std::string &token)
{
	if(token == "0" || token == "off")
		return 0;

	auto it = obj.smoothing_groups.find(token);
	if(it != obj.smoothing_groups.end())
		return it->second;

	int id = obj.next_smoothing_group_id++;
	obj.smoothing_groups[token] = id;
	return id;
}

static int ensure_material(ObjData &obj, const std::string &name)
{
	auto it = obj.used_materials.find(name);
	if(it != obj.used_materials.end())
		return it->second;

	MaterialInfo material;
	material.name = name;

	auto tex_it = obj.material_textures.find(name);
	if(tex_it != obj.material_textures.end())
		material.texture_path = tex_it->second;

	int index = static_cast<int>(obj.materials.size());
	obj.materials.push_back(std::move(material));
	obj.used_materials[name] = index;
	return index;
}

static bool parse_face_vertex(
	const std::string &token,
	const ObjData &obj,
	FaceVertex &face_vertex,
	std::string &error)
{
	size_t slash1 = token.find('/');
	size_t slash2 = (slash1 == std::string::npos) ? std::string::npos : token.find('/', slash1 + 1);

	std::string position_text;
	std::string texcoord_text;
	std::string normal_text;

	if(slash1 == std::string::npos) {
		position_text = token;
	} else {
		position_text = token.substr(0, slash1);
		if(slash2 == std::string::npos) {
			texcoord_text = token.substr(slash1 + 1);
		} else {
			texcoord_text = token.substr(slash1 + 1, slash2 - slash1 - 1);
			normal_text = token.substr(slash2 + 1);
		}
	}

	int raw_index = 0;
	if(position_text.empty() || !try_parse_int(position_text, raw_index)) {
		error = "invalid position index in face token '" + token + "'";
		return false;
	}

	face_vertex.position = resolve_obj_index(raw_index, static_cast<int>(obj.positions.size()));
	if(face_vertex.position < 0 || face_vertex.position >= static_cast<int>(obj.positions.size())) {
		error = "position index out of range in face token '" + token + "'";
		return false;
	}

	if(!texcoord_text.empty()) {
		if(!try_parse_int(texcoord_text, raw_index)) {
			error = "invalid texcoord index in face token '" + token + "'";
			return false;
		}

		face_vertex.texcoord = resolve_obj_index(raw_index, static_cast<int>(obj.texcoords.size()));
		if(face_vertex.texcoord < 0 || face_vertex.texcoord >= static_cast<int>(obj.texcoords.size())) {
			error = "texcoord index out of range in face token '" + token + "'";
			return false;
		}
	}

	if(!normal_text.empty()) {
		if(!try_parse_int(normal_text, raw_index)) {
			error = "invalid normal index in face token '" + token + "'";
			return false;
		}

		face_vertex.normal = resolve_obj_index(raw_index, static_cast<int>(obj.normals.size()));
		if(face_vertex.normal < 0 || face_vertex.normal >= static_cast<int>(obj.normals.size())) {
			error = "normal index out of range in face token '" + token + "'";
			return false;
		}
	}

	return true;
}

static bool load_mtl_file(ObjData &obj, const fs::path &mtl_path, std::string &error)
{
	std::ifstream in(mtl_path);
	if(!in) {
		std::cerr << "Warning: could not open MTL file: " << mtl_path.string() << "\n";
		return true;
	}

	std::string current_material;
	std::string line;
	int line_number = 0;

	while(std::getline(in, line)) {
		line_number++;
		line = trim(line);
		if(line.empty() || line[0] == '#')
			continue;

		if(starts_with(line, "newmtl ")) {
			current_material = get_rest_after_keyword(line);
			if(current_material.empty()) {
				error = "empty material name in " + mtl_path.string() + ":" + std::to_string(line_number);
				return false;
			}
			obj.material_textures.try_emplace(current_material, std::string());
			continue;
		}

		if((starts_with(line, "map_Kd ") || starts_with(line, "map_Ka ")) && !current_material.empty()) {
			std::string texture = get_rest_after_keyword(line);
			if(!texture.empty()) {
				obj.material_textures[current_material] = texture;
				auto used_it = obj.used_materials.find(current_material);
				if(used_it != obj.used_materials.end() && obj.materials[used_it->second].texture_path.empty())
					obj.materials[used_it->second].texture_path = texture;
			}
		}
	}

	return true;
}

static bool add_triangle(
	ObjData &obj,
	const std::vector<FaceVertex> &face_vertices,
	int i0,
	int i1,
	int i2,
	int material_id,
	int smoothing_group)
{
	TriangleBuild triangle {};
	const int indices[3] = { i0, i1, i2 };
	for(int i = 0; i < 3; i++) {
		triangle.tri.v[i].point = face_vertices[indices[i]].position;
		triangle.tri.v[i].uv[0][0] = 0.0f;
		triangle.tri.v[i].uv[0][1] = 0.0f;
		triangle.tri.v[i].uv[0][2] = 0.0f;
		triangle.tri.v[i].uv[0][3] = 0.0f;
		vset4(triangle.tri.v[i].color_rgba, 1.0f, 1.0f, 1.0f, 1.0f);

		if(face_vertices[indices[i]].texcoord >= 0) {
			const auto &uv = obj.texcoords[face_vertices[indices[i]].texcoord];
			triangle.tri.v[i].uv[0][0] = uv[0];
			triangle.tri.v[i].uv[0][1] = 1.0f - uv[1];
		}

		if(face_vertices[indices[i]].normal >= 0) {
			const auto &normal = obj.normals[face_vertices[indices[i]].normal];
			triangle.tri.v[i].normal[0] = normal[0];
			triangle.tri.v[i].normal[1] = normal[1];
			triangle.tri.v[i].normal[2] = normal[2];
			triangle.has_normal[i] = true;
		} else {
			vset3(triangle.tri.v[i].normal, 0.0f, 0.0f, 0.0f);
		}
	}

	triangle.smoothing_group = smoothing_group;
	triangle.tri.surface_tag = reinterpret_cast<const void *>(static_cast<intptr_t>(material_id));

	float edge1[3];
	float edge2[3];
	float cross[3];
	vcopy(edge1, obj.positions[face_vertices[i1].position].data());
	vsub(edge1, obj.positions[face_vertices[i0].position].data());
	vcopy(edge2, obj.positions[face_vertices[i2].position].data());
	vsub(edge2, obj.positions[face_vertices[i0].position].data());
	vcross(cross, edge1, edge2);
	float area2 = vlen(cross);
	if(area2 <= 1e-7f) {
		obj.skipped_degenerate_tris++;
		return true;
	}

	obj.triangles.push_back(triangle);
	return true;
}

static bool load_obj_file(ObjData &obj, const fs::path &obj_path, std::string &error)
{
	std::ifstream in(obj_path);
	if(!in) {
		error = "could not open OBJ file: " + obj_path.string();
		return false;
	}

	int current_material = -1;
	int current_smoothing_group = 0;

	std::string line;
	int line_number = 0;

	while(std::getline(in, line)) {
		line_number++;
		line = trim(line);
		if(line.empty() || line[0] == '#')
			continue;

		if(starts_with(line, "v ")) {
			auto parts = split_whitespace(line);
			if(parts.size() < 4) {
				error = "invalid vertex at line " + std::to_string(line_number);
				return false;
			}

			std::array<float, 3> position {};
			if(!try_parse_float(parts[1], position[0]) ||
			   !try_parse_float(parts[2], position[1]) ||
			   !try_parse_float(parts[3], position[2])) {
				error = "invalid vertex coordinates at line " + std::to_string(line_number);
				return false;
			}

			obj.positions.push_back(position);
			continue;
		}

		if(starts_with(line, "vt ")) {
			auto parts = split_whitespace(line);
			if(parts.size() < 3) {
				error = "invalid texcoord at line " + std::to_string(line_number);
				return false;
			}

			std::array<float, 2> texcoord {};
			if(!try_parse_float(parts[1], texcoord[0]) ||
			   !try_parse_float(parts[2], texcoord[1])) {
				error = "invalid texcoord values at line " + std::to_string(line_number);
				return false;
			}

			obj.texcoords.push_back(texcoord);
			continue;
		}

		if(starts_with(line, "vn ")) {
			auto parts = split_whitespace(line);
			if(parts.size() < 4) {
				error = "invalid normal at line " + std::to_string(line_number);
				return false;
			}

			std::array<float, 3> normal {};
			if(!try_parse_float(parts[1], normal[0]) ||
			   !try_parse_float(parts[2], normal[1]) ||
			   !try_parse_float(parts[3], normal[2])) {
				error = "invalid normal values at line " + std::to_string(line_number);
				return false;
			}

			obj.normals.push_back(normal);
			continue;
		}

		if(starts_with(line, "f ")) {
			auto tokens = split_whitespace(line);
			if(tokens.size() < 4) {
				error = "face has fewer than 3 vertices at line " + std::to_string(line_number);
				return false;
			}

			if(current_material < 0)
				current_material = ensure_material(obj, "default");

			std::vector<FaceVertex> face_vertices;
			face_vertices.reserve(tokens.size() - 1);
			for(size_t i = 1; i < tokens.size(); i++) {
				FaceVertex fv;
				if(!parse_face_vertex(tokens[i], obj, fv, error)) {
					error += " at line " + std::to_string(line_number);
					return false;
				}
				face_vertices.push_back(fv);
			}

			int j = static_cast<int>(face_vertices.size());
			while(j >= 3) {
				j--;
				if(!add_triangle(obj, face_vertices, j, 0, j - 1, current_material, current_smoothing_group))
					return false;
			}
			continue;
		}

		if(starts_with(line, "usemtl ")) {
			std::string material_name = get_rest_after_keyword(line);
			if(material_name.empty()) {
				error = "empty material name at line " + std::to_string(line_number);
				return false;
			}
			current_material = ensure_material(obj, material_name);
			continue;
		}

		if(starts_with(line, "mtllib ")) {
			auto mtllib_tokens = split_whitespace(line);
			for(size_t i = 1; i < mtllib_tokens.size(); i++) {
				fs::path mtl_path = obj_path.parent_path() / fs::path(mtllib_tokens[i]);
				if(!load_mtl_file(obj, mtl_path, error))
					return false;
			}
			continue;
		}

		if(starts_with(line, "s ")) {
			current_smoothing_group = ensure_smoothing_group(obj, get_rest_after_keyword(line));
			continue;
		}
	}

	if(obj.positions.empty()) {
		error = "OBJ contains no vertices";
		return false;
	}

	if(obj.triangles.empty()) {
		error = "OBJ contains no triangles";
		return false;
	}

	return true;
}

static void fill_missing_normals(ObjData &obj)
{
	std::vector<std::array<float, 3>> face_normals(obj.triangles.size());
	std::unordered_map<NormalKey, std::array<float, 3>, NormalKeyHash> accum;

	for(size_t i = 0; i < obj.triangles.size(); i++) {
		const TriangleBuild &triangle = obj.triangles[i];
		float normal[3];
		vtrinormal(
			normal,
			obj.positions[triangle.tri.v[0].point].data(),
			obj.positions[triangle.tri.v[1].point].data(),
			obj.positions[triangle.tri.v[2].point].data());
		face_normals[i] = { normal[0], normal[1], normal[2] };

		for(int j = 0; j < 3; j++) {
			if(triangle.has_normal[j] || triangle.smoothing_group == 0)
				continue;

			NormalKey key { triangle.tri.v[j].point, triangle.smoothing_group };
			auto &sum = accum[key];
			sum[0] += normal[0];
			sum[1] += normal[1];
			sum[2] += normal[2];
		}
	}

	for(auto &entry : accum) {
		float normal[3] = { entry.second[0], entry.second[1], entry.second[2] };
		if(vlen(normal) > 1e-7f) {
			vnormalize(normal);
			entry.second = { normal[0], normal[1], normal[2] };
		} else {
			entry.second = { 0.0f, 1.0f, 0.0f };
		}
	}

	for(size_t i = 0; i < obj.triangles.size(); i++) {
		TriangleBuild &triangle = obj.triangles[i];
		for(int j = 0; j < 3; j++) {
			if(triangle.has_normal[j])
				continue;

			if(triangle.smoothing_group == 0) {
				triangle.tri.v[j].normal[0] = face_normals[i][0];
				triangle.tri.v[j].normal[1] = face_normals[i][1];
				triangle.tri.v[j].normal[2] = face_normals[i][2];
			} else {
				NormalKey key { triangle.tri.v[j].point, triangle.smoothing_group };
				const auto &smoothed = accum[key];
				triangle.tri.v[j].normal[0] = smoothed[0];
				triangle.tri.v[j].normal[1] = smoothed[1];
				triangle.tri.v[j].normal[2] = smoothed[2];
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
	const ObjData &obj,
	st_RawModel &raw_model,
	st_RGeomMtlList &mtl_list,
	std::vector<st_RGeomPart> &parts,
	std::vector<st_RGeomMtlList::st_RGeomMtl> &mtl_storage,
	std::string &error)
{
	std::memset(&raw_model, 0, sizeof(raw_model));
	std::memset(&mtl_list, 0, sizeof(mtl_list));

	raw_model.points_count = static_cast<int>(obj.positions.size());
	raw_model.points = static_cast<float(*)[3]>(std::malloc(sizeof(float[3]) * obj.positions.size()));
	raw_model.tris_count = static_cast<int>(obj.triangles.size());
	raw_model.tris = static_cast<st_RawTriangle *>(std::calloc(obj.triangles.size(), sizeof(st_RawTriangle)));

	if(!raw_model.points || !raw_model.tris) {
		error = "out of memory while building export buffers";
		return false;
	}

	for(size_t i = 0; i < obj.positions.size(); i++) {
		raw_model.points[i][0] = obj.positions[i][0];
		raw_model.points[i][1] = obj.positions[i][1];
		raw_model.points[i][2] = obj.positions[i][2];
	}

	for(size_t i = 0; i < obj.triangles.size(); i++)
		raw_model.tris[i] = obj.triangles[i].tri;

	std::vector<int> material_remap(obj.materials.size(), -1);
	std::vector<int> used_material_ids;
	used_material_ids.reserve(obj.materials.size());
	for(const TriangleBuild &triangle : obj.triangles) {
		int old_material_id = static_cast<int>(reinterpret_cast<intptr_t>(triangle.tri.surface_tag));
		if(old_material_id < 0 || old_material_id >= static_cast<int>(obj.materials.size())) {
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

	for(const TriangleBuild &triangle : obj.triangles) {
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
			error = "material '" + obj.materials[old_material_id].name + "' exceeds 65535 unique vertices";
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
		const MaterialInfo &material = obj.materials[used_material_ids[i]];
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
	std::cerr << "Usage: obj2rgeom <input.obj> [output.rgeom]\n";
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

	ObjData obj;
	std::string error;
	if(!load_obj_file(obj, input_path, error)) {
		std::cerr << "OBJ load failed: " << error << "\n";
		return 1;
	}

	fill_missing_normals(obj);

	st_RawModel raw_model {};
	st_RGeomMtlList mtl_list {};
	std::vector<st_RGeomPart> parts;
	std::vector<st_RGeomMtlList::st_RGeomMtl> mtl_storage;
	RCollision collision {};

	if(!build_export_data(obj, raw_model, mtl_list, parts, mtl_storage, error)) {
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
		&collision);

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
	if(obj.skipped_degenerate_tris > 0)
		std::cout << ", skipped " << obj.skipped_degenerate_tris << " degenerate triangles";
	std::cout << ")\n";

	return 0;
}
