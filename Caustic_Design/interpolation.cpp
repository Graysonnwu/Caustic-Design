#include "interpolation.h"
#include "random.h"
#include "voronoi_creation.h"
#include "config.h"
#include <math.h>

Interpolation::Interpolation(Scene* sc, Scene* tsc, Scene* csc, int sitesAmount, MainWindow* w):m_scene(sc),source_scene(tsc),compute_scene(csc), win(w){
    if (!sc->getDomain().is_valid()) return;
    if (!tsc->getDomain().is_valid()) return;
//    int nbWith = floor(MESH_AMOUNT) / 3;
//    int nbHeight = MESH_AMOUNT / nbWith;
//    FT stepx = 2.0 * tsc->getDomain().get_dx() / nbWith;
//    FT stepy = 2.0 * tsc->getDomain().get_dy() / nbHeight;
//    std::cout << "dx" << tsc->getDomain().get_dy() << std::endl;
//    std::cout << "dy " << tsc->getDomain().get_dy() << std::endl;
//    std::cout << "stepx" << stepx << std::endl;
//    std::cout << "stepy" << stepy << std::endl;
//    int nbpoints = 0;
    int scene_sites = sitesAmount;
//    for (unsigned i = 0; i < nbWith; ++i)
//    {
//        FT x = (i + 0.5)*stepx - tsc->getDomain().get_dx();
//        x += EPS;
//        for (unsigned j = 0; j < nbHeight; ++j)
//        {
//            FT y = (j + 0.5)*stepy - tsc->getDomain().get_dy();
//            y += EPS;
//            Xo.push_back(Point(x, y));
//            source_scene->getLightPointsSource().push_back(Point(x,y));
//            std::cout << "inserted point Xrs x" << x << std::endl;
//            std::cout << "inserted point Xrs y" << y << std::endl;
//            nbpoints++;
//        }
//    }
    std::cout << "nbpoints" << source_scene->getLightPointsSource().size() << std::endl;
    Xo=source_scene->getLightPointsSource();
    //prepare compute scene
    VoronoiCreator voronoi_creator;
    voronoi_creator.generate_voronoi(source_scene, scene_sites, EPSILON);
}


/*
Method 1:
    - insert point
    - recompute voronoi Diagram
    - see which vertices have changed
 to improve: high computation time!

Method 2:
    Use Delaunay triangulation http://cgl.uni-jena.de/pub/Workshops/WebHome/cgl12new.pdf
    Compute neighbirs using CGAL librairy

Method 3:
    Kirkpatrick method http://cgm.cs.mcgill.ca/~athens/cs507/Projects/2002/PaulSandulescu/
    O(logn)

Current: method 1
*/
void Interpolation::runInterpolation(){
   int i;
   for(i=0; i<source_scene->getLightPointsSource().size(); ++i)
   {
        findNaturalNeighbor(Xo[i]);
   }

   // std::vector<Point> computeLightOnDistribution = computeXr(vertices_weight);
   /*next: 1. insert weight
           2. compute weight for all Xo -> compute surface delete point/reset as before insertion
           3. Xo normal distribution
           4. UI for output
    */
   win->viewer->toggle_view_Xrs();
   win->viewer->toggle_view_Xr();
   win->update();
}

void Interpolation::findNaturalNeighbor(Point oP){
    unsigned int i;
    bool newnei;
    int size;
    std::vector<Point> points;
    std::vector<Vertex_handle> neighbors;
    Vertex_handle neighborVertex;

    /*Insert new vertex/oP as centroids as in the computational Scene*/
    std::vector<Vertex_handle>& cs_vertex = compute_scene->getVertices();
    size = compute_scene->getVertices().size();
    std::cout << "vertices size before insertion =" << size << std::endl;
    for (i = 0; i < size; ++i)
    {
       Vertex_handle vi = source_scene->getVertices()[i];
       if (vi->is_hidden()) continue;
       Point ci = vi->compute_centroid();
       points.push_back(ci);
    }
    points.push_back(oP);
    Vertex_handle vertex = compute_scene->insert_vertex(points[i], 0.0, size);
    cs_vertex.push_back(vertex);
    compute_scene->update_positions(points);
    std::vector<FT> weights(points.size(), 0.0);
    compute_scene->construct_triangulation(points, weights);
    neighbors = compute_scene->find_neighbors(compute_scene->getVertices()[size]);

    computeWeights(neighbors, oP);

    /*reset the computational scene*/
    //compute_scene->delete_vertex(vertex);
    points.clear();
    neighbors.clear();
    cs_vertex.pop_back();
    return;
}


void Interpolation::computeWeights(std::vector<Vertex_handle> neighbors, Point oP){
    unsigned int i;
    unsigned int mscIndex, cscIndex;
    std::vector<std::pair<Vertex_handle, FT> > vertices_weight;
    std::pair<Vertex_handle, FT> p;
    Vertex_handle vn;
    double x = 0;
    double y = 0;
//    FT areaOnFace;
//    FT weight;
//    FT temp;

    if (neighbors.size() == 0) {
        std::cout << "weight vector empty" << std::endl;
        return;
    }

    /*Compute the ratio of area occupied by the new cell on the older cell*/
    for (i=0; i<neighbors.size(); ++i){
        /*compute area of the overlapsing cell*/
        vn = neighbors[i];
        mscIndex = source_scene->findIndexVerticeBySite(vn);
        cscIndex = compute_scene->findIndexVerticeByCentroid(vn);
        std::cout<<"index source scene"<<mscIndex<<std::endl;
        std::cout<<"index compute scene"<<cscIndex<<std::endl;

        // FT area_source_scene = source_scene->getVertices()[mscIndex]->compute_area();
       // FT area_c_scene = compute_scene->getVertices()[cscIndex]->compute_area();

        FT area_source_scene = source_scene->getVertices()[cscIndex]->compute_area();
        FT area_c_scene = compute_scene->getVertices()[cscIndex]->compute_area();
        std::cout<<"area_source_scene: "<<area_source_scene<<std::endl;
        std::cout<<"area_c_scene: "<<area_c_scene<<std::endl;

        FT areaOnFace= area_source_scene-area_c_scene;
        /*compute ratio*/
        FT temp = source_scene->getVertices()[mscIndex]->compute_area();
        FT weight = areaOnFace/temp;
        std::cout<<"weight"<<weight<<std::endl;
        /*retrieve on m_scene*/
        x = x + (m_scene->getVertices()[mscIndex]->compute_centroid().x())*weight;
        y = y + (m_scene->getVertices()[mscIndex]->compute_centroid().y())*weight;
    }
    std::cout<<"x for Xr"<<x<<std::endl;
    std::cout<<"y for Xr"<<y<<std::endl;
    m_scene->getLightPointsTarget().push_back(Point(x,y));
    m_scene->getLightPointsTarget().push_back(Point(x,y));
    source_scene->getLightPointsTarget().push_back(Point(x,y));
    source_scene->getLightPointsTarget().push_back(Point(x,y));

    return;
}

