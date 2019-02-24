//=============================================================================
// Copyright (C) 2011-2019 The pmp-library developers
//
// This file is part of the Polygon Mesh Processing Library.
// Distributed under the terms of the MIT license, see LICENSE.txt for details.
//
// SPDX-License-Identifier: MIT
//=============================================================================

#include "SurfaceNormals.h"

//=============================================================================

namespace pmp {

//=============================================================================

Normal SurfaceNormals::compute_vertex_normal(const SurfaceMesh& mesh, Vertex v)
{
    Point nn(0, 0, 0);
    Halfedge h = mesh.halfedge(v);

    if (h.is_valid())
    {
        auto vpoint = mesh.get_vertex_property<Point>("v:point");

        const Halfedge hend = h;
        const Point p0 = vpoint[v];

        Point n, p1, p2;
        Scalar cosine, angle, denom;

        do
        {
            if (!mesh.is_boundary(h))
            {
                p1 = vpoint[mesh.to_vertex(h)];
                p1 -= p0;

                p2 = vpoint[mesh.from_vertex(mesh.prev_halfedge(h))];
                p2 -= p0;

                // check whether we can robustly compute angle
                denom = sqrt(dot(p1, p1) * dot(p2, p2));
                if (denom > std::numeric_limits<Scalar>::min())
                {
                    cosine = dot(p1, p2) / denom;
                    if (cosine < -1.0)
                        cosine = -1.0;
                    else if (cosine > 1.0)
                        cosine = 1.0;
                    angle = acos(cosine);

                    n = cross(p1, p2);

                    // check whether normal is != 0
                    denom = norm(n);
                    if (denom > std::numeric_limits<Scalar>::min())
                    {
                        n *= angle / denom;
                        nn += n;
                    }
                }
            }

            h = mesh.cw_rotated_halfedge(h);
        } while (h != hend);

        nn = normalize(nn);
    }

    return nn;
}

//-----------------------------------------------------------------------------

Normal SurfaceNormals::compute_face_normal(const SurfaceMesh& mesh, Face f)
{
    Halfedge h = mesh.halfedge(f);
    Halfedge hend = h;

    auto vpoint = mesh.get_vertex_property<Point>("v:point");

    Point p0 = vpoint[mesh.to_vertex(h)];
    h = mesh.next_halfedge(h);
    Point p1 = vpoint[mesh.to_vertex(h)];
    h = mesh.next_halfedge(h);
    Point p2 = vpoint[mesh.to_vertex(h)];

    if (mesh.next_halfedge(h) == hend) // face is a triangle
    {
        return normalize(cross(p2 -= p1, p0 -= p1));
    }
    else // face is a general polygon
    {
        Normal n(0, 0, 0);

        hend = h;
        do
        {
            n += cross(p2 - p1, p0 - p1);
            h = mesh.next_halfedge(h);
            p0 = p1;
            p1 = p2;
            p2 = vpoint[mesh.to_vertex(h)];
        } while (h != hend);

        return normalize(n);
    }
}

//-----------------------------------------------------------------------------

Normal SurfaceNormals::compute_corner_normal(const SurfaceMesh& mesh,
                                             Halfedge h, Scalar crease_angle)
{
    // catch the two trivial cases
    if (crease_angle < 0.01)
        return compute_face_normal(mesh, mesh.face(h));
    else if (crease_angle > 179)
        return compute_vertex_normal(mesh, mesh.from_vertex(h));

    // avoid numerical problems
    if (crease_angle < 0.001)
        crease_angle = 0.001;

    const Scalar cos_crease_angle = cos(crease_angle);
    Point nn(0, 0, 0);

    if (!mesh.is_boundary(h))
    {
        auto vpoint = mesh.get_vertex_property<Point>("v:point");

        const Halfedge hend = h;
        const Vertex v0 = mesh.to_vertex(h);
        const Point p0 = vpoint[v0];

        Point n, p1, p2;
        Scalar cosine, angle, denom;

        // compute normal of h's face
        p1 = vpoint[mesh.to_vertex(mesh.next_halfedge(h))];
        p1 -= p0;
        p2 = vpoint[mesh.from_vertex(h)];
        p2 -= p0;
        const Point nf = normalize(cross(p1, p2));

        // average over all incident faces
        do
        {
            if (!mesh.is_boundary(h))
            {
                p1 = vpoint[mesh.to_vertex(mesh.next_halfedge(h))];
                p1 -= p0;
                p2 = vpoint[mesh.from_vertex(h)];
                p2 -= p0;

                n = cross(p1, p2);

                // check whether normal is != 0
                denom = norm(n);
                if (denom > std::numeric_limits<Scalar>::min())
                {
                    n /= denom;

                    // check whether normal is withing crease_angle bound
                    if (dot(n, nf) >= cos_crease_angle)
                    {
                        // check whether we can robustly compute angle
                        denom = sqrt(dot(p1, p1) * dot(p2, p2));
                        if (denom > std::numeric_limits<Scalar>::min())
                        {
                            cosine = dot(p1, p2) / denom;
                            if (cosine < -1.0)
                                cosine = -1.0;
                            else if (cosine > 1.0)
                                cosine = 1.0;
                            angle = acos(cosine);

                            n *= angle;
                            nn += n;
                        }
                    }
                }
            }

            h = mesh.opposite_halfedge(mesh.next_halfedge(h));
        } while (h != hend);

        nn = normalize(nn);
    }

    return nn;
}

//-----------------------------------------------------------------------------

void SurfaceNormals::compute_vertex_normals(SurfaceMesh& mesh)
{
    auto vnormal = mesh.vertex_property<Normal>("v:normal");
    for (auto v : mesh.vertices())
        vnormal[v] = compute_vertex_normal(mesh, v);
}

//-----------------------------------------------------------------------------

void SurfaceNormals::compute_face_normals(SurfaceMesh& mesh)
{
    auto fnormal = mesh.face_property<Normal>("f:normal");
    for (auto f : mesh.faces())
        fnormal[f] = compute_face_normal(mesh, f);
}

//=============================================================================
} // namespace pmp
//=============================================================================
