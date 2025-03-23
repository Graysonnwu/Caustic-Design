// This file is part of otmap, an optimal transport solver.
//
// Copyright (C) 2017-2018 Gael Guennebaud <gael.guennebaud@inria.fr>
// Copyright (C) 2017 Georges Nader
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <future>
#include <fstream>
#include <sstream>
#include <string>

#include "normal_integration/normal_integration.h"
#include "normal_integration/mesh.h"

#include "optimal_transport/scene.h"
#include "optimal_transport/optimal_transport.h"
#include "optimal_transport/interpolation.h"
#include "optimal_transport/config.h"


std::unordered_map<std::string, std::string> parse_arguments(int argc, char const *argv[]) {
    // Define a map to store the parsed arguments
    std::unordered_map<std::string, std::string> args;

    // Iterate through command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string key, value;

        // Check if argument starts with '--'
        if (arg.substr(0, 2) == "--") {
            // Split argument by '=' to separate key and value
            size_t pos = arg.find('=');
            if (pos != std::string::npos) {
                key = arg.substr(2, pos - 2);
                value = arg.substr(pos + 1);
            }
            else {
                key = arg.substr(2);
                value = ""; // No value provided
            }
        }
        // Check if argument starts with '-'
        else if (arg[0] == '-') {
            // The next argument is the value
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                key = arg.substr(1);
                value = argv[++i];
            }
            else {
                key = arg.substr(1);
                value = ""; // No value provided
            }
        }
        // Invalid argument format
        else {
            //std::cerr << "Invalid argument format: " << arg << std::endl;
            //return 1;
        }

        // Store key-value pair in the map
        args[key] = value;
    }

    return args;
}

// interpolate target mesh into a rectangular grid
std::vector<double> interpolate_point(Mesh &mesh, const std::vector<std::vector<double>>& positions, std::vector<double> &point, bool &triangle_miss) {
    Hit hit;
    bool intersection = false;
    mesh.source_bvh->query(point, hit, intersection);
    if (intersection) {
        double interpolation_x = positions[mesh.triangles[hit.face_id][0]][0]*hit.barycentric_coords[0] + 
                                 positions[mesh.triangles[hit.face_id][1]][0]*hit.barycentric_coords[1] + 
                                 positions[mesh.triangles[hit.face_id][2]][0]*hit.barycentric_coords[2];
        double interpolation_y = positions[mesh.triangles[hit.face_id][0]][1]*hit.barycentric_coords[0] + 
                                 positions[mesh.triangles[hit.face_id][1]][1]*hit.barycentric_coords[1] + 
                                 positions[mesh.triangles[hit.face_id][2]][1]*hit.barycentric_coords[2];
        triangle_miss = false;
        return {interpolation_x, interpolation_y, 0.0f};
    } else {
        printf("interpolation miss!\r\n");
        printf("x: %f, y: %f\r\n", point[0], point[1]);
        //exit(0);
        triangle_miss = true;
        return point;
    }

    //printf("interpolation miss!\r\n");

    return {NAN, NAN};
}

std::vector<double> calculate_centroid_vector(std::vector<Point> vertices) {
    std::vector<double> centroid;
    centroid.push_back(0.0);
    centroid.push_back(0.0);

    double signed_area = 0;

    for (int i = 0; i < vertices.size(); i++) {
        double x0 = vertices[i].x();
        double y0 = vertices[i].y();
        double x1 = vertices[(i + 1) % vertices.size()].x();
        double y1 = vertices[(i + 1) % vertices.size()].y();

        // Shoelace formula
        double area = (x0 * y1) - (x1 * y0);
        signed_area += area;
        centroid[0] += (x0 + x1) * area;
        centroid[1] += (y0 + y1) * area;
    }

    signed_area *= 0.5;
    centroid[0] /= 6 * signed_area;
    centroid[1] /= 6 * signed_area;

    return centroid;
}

// 多线程插值处理函数
void interpolate_points_range(
    const Mesh& interpolation_mesh,
    const std::vector<std::vector<double>>& pd_centroids,
    const std::vector<std::vector<double>>& source_points,
    std::vector<std::vector<double>>& target_points,
    size_t start_idx, 
    size_t end_idx,
    std::mutex& print_mutex) {
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        if (i % 1000 == 0) {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "thread " << std::this_thread::get_id() << " interpolation: " << i - start_idx << "/" << end_idx - start_idx << std::endl;
        }
        
        bool triangle_miss = false;
        std::vector<double> interpolated_point = interpolate_point(
            const_cast<Mesh&>(interpolation_mesh), 
            pd_centroids, 
            const_cast<std::vector<double>&>(source_points[i]), 
            triangle_miss);
        
        interpolated_point.push_back(0.0f); // 添加z值
        target_points[i] = interpolated_point;
    }
}

int main(int argc, char const *argv[])
{
    setlocale(LC_ALL,"C");

    // Parse user arguments
    std::unordered_map<std::string, std::string> args = parse_arguments(argc, argv);
    
    // example usage: ./interpolate --image=../source.png --points=../mita.dat --weights=../mita.weight --mesh_res=500
    int mesh_res = 500;
    std::string image_filename = "";
    std::string points_filename = "";
    std::string weights_filename = "";
    std::string output_prefix = "";

    mesh_res = std::stoi(args["mesh_res"]);
    image_filename = args["image"].c_str(); // any image would do, we just need the size
    points_filename = args["points"];
    weights_filename = args["weights"];

    if (args.find("output") != args.end()) {
        output_prefix = args["output"];
    } else { // 默认使用points文件名作为输出前缀
        output_prefix = points_filename.substr(0, points_filename.find_last_of('.'));
    }

    std::cout << "Reading points from: " << points_filename << std::endl;
    std::cout << "Reading weights from: " << weights_filename << std::endl;
    std::cout << "Mesh resolution: " << mesh_res << std::endl;
    std::cout << "Output prefix: " << output_prefix << std::endl;
    std::cout << "Source image: " << image_filename << std::endl;

    // 读取点和权重
    std::vector<Point> o_points;
    std::vector<FT> o_weights;

    // 读取点文件
    std::ifstream points_file(points_filename);
    if (!points_file.is_open()) {
        std::cerr << "Failed to open points file: " << points_filename << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(points_file, line)) {
        std::istringstream iss(line);
        double x, y;
        if (iss >> x >> y) {
            o_points.push_back(Point(x, y));
        }
    }
    points_file.close();

    // 读取权重文件
    std::ifstream weights_file(weights_filename);
    if (!weights_file.is_open()) {
        std::cerr << "Failed to open weights file: " << weights_filename << std::endl;
        return 1;
    }

    while (std::getline(weights_file, line)) {
        std::istringstream iss(line);
        double weight;
        if (iss >> weight) {
            o_weights.push_back(FT(weight));
        }
    }
    weights_file.close();

    if (o_points.size() != o_weights.size()) {
        std::cerr << "Error: Number of points (" << o_points.size() 
                  << ") does not match number of weights (" << o_weights.size() << ")" << std::endl;
        return 1;
    }

    std::cout << "Loaded " << o_points.size() << " points and weights." << std::endl;

    // add dummy points
    o_points.push_back(Point(1000, 1000)); o_points.push_back(Point(-1000, 1000)); o_points.push_back(Point(1000, -1000)); o_points.push_back(Point(-1000, -1000));
    o_weights.push_back(FT(0.0)); o_weights.push_back(FT(0.0)); o_weights.push_back(FT(0.0)); o_weights.push_back(FT(0.0));
    std::cout << "After adding dummy points, there are " << o_points.size() << " points and weights." << std::endl;

    // 创建Scene对象
    Scene* source_scene = new Scene;
    source_scene->load_image(image_filename);
    source_scene->construct_triangulation(o_points, o_weights);
    std::cout << "m_vertices.size() = " << source_scene->m_vertices.size() << std::endl;

    std::vector<std::vector<double>> points;
    std::vector<std::vector<double>> pd_centroids;
    std::vector<std::vector<unsigned int>> triangles;

    for (auto fit = source_scene->m_rt.finite_faces_begin(); fit != source_scene->m_rt.finite_faces_end(); ++fit) {
        std::vector<unsigned int> triangle = {
            static_cast<unsigned int>(fit->vertex(0)->get_index()),
            static_cast<unsigned int>(fit->vertex(1)->get_index()),
            static_cast<unsigned int>(fit->vertex(2)->get_index())
        };
        triangles.push_back(triangle);
    }

    for (int i = 0; i < source_scene->m_vertices.size(); i++)
    {
        points.push_back({
            source_scene->m_vertices[i]->get_position().x() + 0.5,
            source_scene->m_vertices[i]->get_position().y() + 0.5
        });
    }

    for (unsigned i = 0; i < source_scene->m_vertices.size(); ++i) 
    {
        Vertex_handle vi = source_scene->m_vertices[i];
        std::vector<Point> polygon;

        if (vi->is_hidden()) continue;
        source_scene->m_rt.build_polygon(vi, polygon);

        pd_centroids.push_back(calculate_centroid_vector(polygon));
        pd_centroids[i][0] += 0.5;
        pd_centroids[i][1] += 0.5;
    }
    
    Mesh mesh(1.0, 1.0, mesh_res, mesh_res);
    mesh.build_vertex_to_triangles();
    mesh.calculate_vertex_laplacians();

    Mesh interpolation_mesh(points, triangles);

    interpolation_mesh.build_source_bvh(5, 30);
    std::vector<std::vector<double>> target_points(mesh.source_points.size());
    
    // 计算CPU线程数量并创建线程
    unsigned int num_threads = std::thread::hardware_concurrency();
    // 至少使用2个线程，最多8个线程
    num_threads = std::max(2u, std::min(8u, num_threads));
    std::cout << "Using " << num_threads << " threads for interpolation." << std::endl;
    
    std::vector<std::thread> threads;
    std::mutex print_mutex;
    
    // 计算每个线程处理的点数量
    size_t points_per_thread = mesh.source_points.size() / num_threads;
    
    // 创建并启动线程
    for (unsigned int t = 0; t < num_threads; ++t) {
        size_t start_idx = t * points_per_thread;
        size_t end_idx = (t == num_threads - 1) ? mesh.source_points.size() : (t + 1) * points_per_thread;
        
        threads.emplace_back(
            interpolate_points_range,
            std::ref(interpolation_mesh),
            std::ref(pd_centroids),
            std::ref(mesh.source_points),
            std::ref(target_points),
            start_idx,
            end_idx,
            std::ref(print_mutex)
        );
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "Multi-thread interpolation done." << std::endl;

    // 输出插值结果
    std::string otmap_filename = output_prefix + "_" + std::to_string(mesh_res) + "-otmap.dat";
    std::ofstream outfile(otmap_filename);
    outfile.precision(10);
    outfile << std::fixed;
    for(size_t i = 0; i < mesh.source_points.size(); i++) {
        outfile << mesh.source_points[i][0] << " " << mesh.source_points[i][1] << " "<< target_points[i][0] << " " << target_points[i][1] << std::endl;
    }
    outfile.close();
    std::cout << "Output points to " << otmap_filename << std::endl;

    delete source_scene;
    return 0;
} 