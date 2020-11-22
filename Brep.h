#pragma once

#include <vector>
#include <iostream>

struct Point;
struct Vertex;
struct HalfEdge;
struct Edge;
struct Loop;
struct Face;
struct Solid;

using namespace std;

struct Solid {
	Solid() : SolidId(num++) {}
	int SolidId;
	vector<Face*> faces;
	vector<Edge*> edges;
	vector<Vertex*> vertices;

	static int num;
};

int Solid::num = 0;

struct Point {
	Point() = default;
	Point(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}

	friend ifstream& operator >>(ifstream& input, Point& p) {
		input >> p.x >> p.y >> p.z;
		return input;
	}

	friend ostream& operator <<(ostream& output, Point& p) {
		output << p.x << " " << p.y << " " <<  p.z << endl;
		return output;
	}

	double x = 0;
	double y = 0;
	double z = 0;
};

struct Vertex {
	Vertex() = default;

	Vertex(Point* _point, Solid* solid) : VertexId(num++), point(_point) {
		solid->vertices.push_back(this);
	}

	Vertex(double _x, double _y, double _z, Solid* solid): VertexId(num++), point(new Point(_x, _y, _z)) {
		solid->vertices.push_back(this);
	}

	int VertexId;
	Point* point = new Point;
	static int num;

	void debug() {
		printf("Vertex debug: %f %f %f\n", point->x, point->y, point->z);
	}
};

int Vertex::num = 0;

struct HalfEdge {
	HalfEdge() = default;
	HalfEdge(Vertex* v1, Vertex* v2): start(v1), end(v2) {}

	Loop* loop = nullptr;
	Edge* edge = nullptr;
	Vertex* start = nullptr;
	Vertex* end = nullptr;
	HalfEdge* partner = nullptr;

	HalfEdge* next = nullptr;
	HalfEdge* pre = nullptr;
};

struct Edge {
	Edge() = default;

	Edge(HalfEdge* _he1, HalfEdge* _he2, Solid* solid): he1(_he1), he2(_he2), EdgeId(num++) {
		he1->partner = he2;
		he2->partner = he1;
		he1->edge = he2->edge = this;
		solid->edges.push_back(this);
	}

	int EdgeId;

	HalfEdge* he1 = nullptr;
	HalfEdge* he2 = nullptr;

	static int num;
};

int Edge::num = 0;

struct Loop {
	Loop(): loopId(num++) {}
	Loop(Face* _face): loopId(num++), face(_face) {}

	int loopId;
	Face* face = nullptr;
	HalfEdge* first_edge = nullptr;

	static int num;
};

int Loop::num = 0;

struct Face {
	Face() = default;

	Face(Loop* _outloop, Solid* _solid) : faceId(num++), solid(_solid), outer_loop(_outloop) {
		outer_loop->face = this;
		solid->faces.push_back(this);
	}

	int faceId;
	Solid* solid = nullptr;
	Loop* outer_loop = nullptr;
	vector<Loop*> inner_loops;

	static int num;
};

int Face::num = 0;

inline void debug(HalfEdge* he) {
	printf("(%5.2f,%5.2f,%5.2f) -> (%5.2f,%5.2f,%5.2f), edgeid: %d, loopid: %d\n", he->start->point->x,
	       he->start->point->y, he->start->point->z, he->end->point->x, he->end->point->y, he->end->point->z,
	       he->edge->EdgeId, he->loop->loopId);
}

inline void debug(Edge* e) {
	printf("EdgeId: %d\n", e->EdgeId);
	printf("h1: ");
	debug(e->he1);
	printf("h2: ");
	debug(e->he2);
}

inline void debug(Loop* loop) {
	printf("Loop debug:\n");
	auto* he = loop->first_edge;
	do {
		debug(he);
		he = he->next;
	} while (he != loop->first_edge);
}

inline void debug(Face* face) {
	printf("Face %d:\n", face->faceId);
	printf("outer_loop:\n");
	debug(face->outer_loop);
	int i = 0;
	for (auto inner_loop : face->inner_loops) {
		printf("inner_loop: %d\n", i++);
		debug(inner_loop);
	}
}

inline void debug(Solid* solid) {
	for (auto face : solid->faces) {
		debug(face);
	}
}


struct Brep {
	vector<Solid*> solids;

	Vertex* MVFS(double x, double y, double z) {
		Solid* solid = new Solid;
		Vertex* vertex = new Vertex(x, y, z, solid);
		Loop* loop = new Loop;
		new Face(loop, solid);
		solids.push_back(solid);
		return vertex;
	}

	Vertex* MEV(Loop* loop, Vertex* v1, double x, double y, double z) {
		Solid* solid = loop->face->solid;
		Vertex* v2 = new Vertex(x, y, z, solid);
		MEV(loop, v1, v2);
		return v2;
	}

	Vertex* MEV(Loop* loop, Vertex* v1, Vertex* v2) {
		Solid* solid = loop->face->solid;
		HalfEdge* he1 = new HalfEdge(v1, v2);
		HalfEdge* he2 = new HalfEdge(v2, v1);
		Edge* edge = new Edge(he1, he2, solid);
		he1->loop = he2->loop = loop;

		he1->next = he2;
		he2->pre = he1;

		if (loop->first_edge == nullptr) {
			he2->next = he1;
			he1->pre = he2;
			loop->first_edge = he1;
		} else {
			HalfEdge* he;
			for (he = loop->first_edge; he->end != v1; he = he->next);
			he2->next = he->next;
			he2->next->pre = he2;
			he->next = he1;
			he1->pre = he;
		}
		// debug(loop);
		return v2;
	}

	Face* MEF(Loop* loop, Vertex* e1_start, Vertex* e1_end, Vertex* e2_start, Vertex* e2_end) {
		Solid* solid = loop->face->solid;
		HalfEdge *he1, *he2;
		for (he1 = loop->first_edge; he1->start != e1_start || he1->end != e1_end; he1 = he1->next);
		for (he2 = he1; he2->start != e2_start || he2->end != e2_end; he2 = he2->next);

		HalfEdge* new_he1 = new HalfEdge(e1_end, e2_end);
		HalfEdge* new_he2 = new HalfEdge(e2_end, e1_end);
		new Edge(new_he1, new_he2, solid);

		new_he1->next = he2->next;
		new_he1->pre = he1;
		new_he2->next = he1->next;
		new_he2->pre = he2;
		new_he1->next->pre = new_he1;
		new_he1->pre->next = new_he1;
		new_he2->next->pre = new_he2;
		new_he2->pre->next = new_he2;

		Loop* new_loop = new Loop;
		Face* new_face = new Face(new_loop, solid);

		new_loop->first_edge = new_he1;
		new_he1->loop = new_loop;
		loop->first_edge = new_he2;
		new_he2->loop = loop;

		HalfEdge* he = new_he1->next;
		while (he != new_he1) {
			he->loop = new_loop;
			he = he->next;
		}
		// printf("old face: \n");
		// debug(loop);
		// printf("new face: \n");
		// debug(new_loop);

		return new_face;
	}

	Loop* KEMR(Loop* loop, Vertex* v1, Vertex* v2) {
		HalfEdge* he1;
		for (he1 = loop->first_edge; !(he1->start == v1 && he1->end == v2); he1 = he1->next);
		HalfEdge* he2 = he1->partner;

		he1->next->pre = he2->pre;
		he1->pre->next = he2->next;
		he2->next->pre = he1->pre;
		he2->pre->next = he1->next;

		Face* face = loop->face;
		Loop* new_loop = new Loop(face);
		face->inner_loops.push_back(new_loop);

		loop->first_edge = he1->next;
		new_loop->first_edge = he2->next;
		he2->next->loop = new_loop;

		HalfEdge* he = new_loop->first_edge->next;
		while (he != new_loop->first_edge) {
			he->loop = new_loop;
			he = he->next;
		}

		HalfEdge* tmp = loop->first_edge;
		auto& edge_list = loop->face->solid->edges;
		auto edge_it = std::find(edge_list.begin(), edge_list.end(), he1->edge);
		edge_list.erase(edge_it);
		delete he1->edge;
		delete he1;
		delete he2;
		// printf("KEMR outloop\n");
		// debug(loop);
		// for (int i = 0; i < face->inner_loops.size(); ++i) {
		// 	printf("innerloop %d\n", i);
		// 	debug(face->inner_loops[i]);
		// }
		return new_loop;
	}

	Solid* KFMRH(Face* outer_face, Face* inner_face) {
		Solid* solid1 = outer_face->solid;
		Solid* solid2 = inner_face->solid;
		auto& face_list = solid1->faces;
		if (solid1 == solid2) {
			outer_face->inner_loops.push_back(inner_face->outer_loop);
			// debug(outer_face);
			inner_face->outer_loop->face = outer_face;
			// debug(inner_face);
			auto face_it = std::find(face_list.begin(), face_list.end(), inner_face);
			face_list.erase(face_it);
			delete inner_face;
		} else {
			// todo: combine solids
		}
		return solid1;
	}

	Solid* sweep(Face* face, double dx, double dy, double dz) {
		Solid* solid = face->solid;
		Loop *loop = face->outer_loop, *enclosed_loop = loop->first_edge->partner->loop;
		Face* enclosed_face = enclosed_loop->face;
		HalfEdge* first_edge = loop->first_edge;
		Vertex *first_vertex = first_edge->start, *last_start = first_vertex;
		Vertex *first_new_vertex = new Vertex(first_vertex->point->x + dx, first_vertex->point->y + dy,
		                                      first_vertex->point->z + dz, solid), *last_end = first_new_vertex;
		MEV(enclosed_loop, first_vertex, first_new_vertex);
		for (HalfEdge* he = first_edge->pre; he != first_edge; he = he->pre) {
			Vertex *start = he->start, *end = new Vertex(start->point->x + dx, start->point->y + dy,
			                                             start->point->z + dz, solid);
			MEV(enclosed_loop, start, end);
			MEF(enclosed_loop, start, end, last_start, last_end);
			last_start = last_end;
			last_end = end;
		}
		MEF(enclosed_loop, first_vertex, first_new_vertex, last_start, last_end);
		for (Loop* inner_loop : face->inner_loops) {
			first_edge = inner_loop->first_edge;
			enclosed_loop = first_edge->partner->loop;
			first_vertex = first_edge->start;
			last_start = first_vertex;
			first_new_vertex = new Vertex(first_vertex->point->x + dx, first_vertex->point->y + dy,
			                              first_vertex->point->z + dz, solid);
			last_end = first_new_vertex;
			MEV(enclosed_loop, first_vertex, first_new_vertex);
			// debug(enclosed_loop);
			for (HalfEdge* he = first_edge->pre; he != first_edge; he = he->pre) {
				Vertex *start = he->start, *end = new Vertex(start->point->x + dx, start->point->y + dy,
				                                             start->point->z + dz, solid);
				MEV(enclosed_loop, start, end);
				MEF(enclosed_loop, start, end, last_start, last_end);
				last_start = last_end;
				last_end = end;
			}
			MEF(enclosed_loop, first_vertex, first_new_vertex, last_start, last_end);
			KFMRH(enclosed_face, enclosed_loop->face);
		}
		return solid;
	}
};
