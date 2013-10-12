/*
 Copyright (C) 2010-2013 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TrenchBroom_BBox_h
#define TrenchBroom_BBox_h

#include "Mat.h"
#include "Plane.h"
#include "Quat.h"
#include "Vec.h"

template <typename T, size_t S>
class BBox {
public:
    typedef enum {
        Min,
        Max
    } MinMax;

    class RelativePosition {
    public:
        typedef enum {
            Less,
            Within,
            Greater
        } Range;
    private:
        Range m_positions[S];
    public:
        RelativePosition(const Range positions[S]) {
            for (size_t i = 0; i < S; ++i)
                m_positions[i] = positions[i];
        }
        
        const Range& operator[] (const size_t index) const {
            assert(index >= 0 && index < S);
            return m_positions[index];
        }
    };
    
    Vec<T,S> min;
    Vec<T,S> max;
    
    BBox() :
    min(Vec<T,S>::Null),
    max(Vec<T,S>::Null) {}
    
    BBox(const Vec<T,S>& i_min, const Vec<T,S>& i_max) :
    min(i_min),
    max(i_max) {}
    
    BBox(const T i_minMax) {
        min.set(-i_minMax);
        max.set(+i_minMax);
    }
    
    BBox(const T i_min, const T i_max) {
        min.set(i_min);
        max.set(i_max);
    }
    
    BBox(const Vec<T,S>& center, const T size) {
        for (size_t i = 0; i < S; i++) {
            min[i] = center[i] - size;
            max[i] = center[i] + size;
        }
    }
    
    BBox(const typename Vec<T,S>::List& vertices) {
        assert(vertices.size() > 0);
        min = max = vertices[0];
        for (size_t i = 0; i < vertices.size(); ++i)
            mergeWith(vertices[i]);
    }
    
    template <typename U>
    BBox(const BBox<U,S>& other) :
    min(other.min),
    max(other.max) {}

    bool operator== (const BBox<T,S>& right) const {
        return min == right.min && max == right.max;
    }
    
    const Vec<T,S> center() const {
        return (min + max) / static_cast<T>(2.0);
    }
    
    const Vec<T,S> size() const {
        return max - min;
    }
    
    const Vec<T,S> vertex(MinMax c[S]) const {
        Vec<T,S> result;
        for (size_t i = 0; i < S; i++)
            result[i] = c[i] == Min ? min[i] : max[i];
        return result;
    }
    
    const Vec<T,3> vertex(MinMax x, MinMax y, MinMax z) const {
        MinMax c[] = {x, y, z};
        return vertex(c);
    }
    
    BBox<T,S>& mergeWith(const BBox<T,S>& right) {
        for (size_t i = 0; i < S; i++) {
            min[i] = std::min(min[i], right.min[i]);
            max[i] = std::max(max[i], right.max[i]);
        }
        return *this;
    }
    
    const BBox<T,S> mergedWith(const BBox<T,S>& right) const {
        return BBox<T,S>(*this).mergeWith(right);
    }
    
    
    BBox<T,S>& mergeWith(const Vec<T,S>& right) {
        for (size_t i = 0; i < S; i++) {
            min[i] = std::min(min[i], right[i]);
            max[i] = std::max(max[i], right[i]);
        }
        return *this;
    }
    
    const BBox<T,S> mergedWith(const Vec<T,S>& right) const {
        return BBox<T,S>(*this).mergeWith(right);
    }
    
    BBox<T,S>& translateToOrigin() {
        const Vec<T,S> c = center();
        min -= c;
        max -= c;
        return *this;
    }
    
    const BBox<T,S> translatedToOrigin() const {
        return BBox<T,S>(*this).translateToOrigin();
    }
    
    BBox<T,S>& repair() {
        for (size_t i = 0; i < S; i++)
            if (min[i] > max[i])
                std::swap(min[i], max[i]);
        return *this;
    }
    
    const BBox<T,S> repaired() const {
        return BBox<T,S>(*this).repair();
    }

    bool contains(const Vec<T,S>& point) const {
        for (size_t i = 0; i < S; i++)
            if (point[i] < min[i] || point[i] > max[i])
                return false;
        return true;
    }
    
    const RelativePosition relativePosition(const Vec<T,S>& point) const {
        typename RelativePosition::Range p[S];
        for (size_t i = 0; i < S; i++) {
            if (point[i] < min[i])
                p[i] = RelativePosition::Less;
            else if (point[i] > max[i])
                p[i] = RelativePosition::Greater;
            else
                p[i] = RelativePosition::Within;
        }
        
        return RelativePosition(p);
    }

    bool contains(const BBox<T,S>& bounds) const {
        for (size_t i = 0; i < S; i++)
            if (bounds.min[i] < min[i] || bounds.max[i] > max[i])
                return false;
        return true;
    }
    
    bool intersects(const BBox<T,S>& bounds) const {
        for (size_t i = 0; i < S; i++)
            if (bounds.max[i] < min[i] || bounds.min[i] > max[i])
                return false;
        return true;
        
    }
    
    T intersectWithRay(const Ray<T,S>& ray, Vec<T,S>* sideNormal = NULL) const {
        const bool inside = contains(ray.origin);
        
        for (size_t i = 0; i < S; ++i) {
            if (ray.direction[i] == static_cast<T>(0.0))
                continue;
            
            Vec<T,S> normal, position;
            normal[i] = ray.direction[i] < static_cast<T>(0.0) ? static_cast<T>(1.0) : static_cast<T>(-1.0);
            if (inside)
                position = ray.direction[i] < static_cast<T>(0.0) ? min : max;
            else
                position = ray.direction[i] < static_cast<T>(0.0) ? max : min;
            
            const Plane<T,S> plane(position, normal);
            const T distance = plane.intersectWithRay(ray);
            if (Math::isnan(distance))
                continue;
            
            const Vec<T,S> point = ray.pointAtDistance(distance);
            for (size_t j = 0; j < S; ++j)
                if (i != j && !Math::between(point[j], min[j], max[j]))
                    goto cont;
            
            if (sideNormal != NULL)
                *sideNormal = inside ? -normal : normal;
            return distance;
            
        cont:;
        }
        
        return std::numeric_limits<T>::quiet_NaN();
    }
    
    BBox<T,S>& expand(const T f) {
        for (size_t i = 0; i < S; i++) {
            min[i] -= f;
            max[i] += f;
        }
        return *this;
    }
    
    const BBox<T,S> expanded(const T f) const {
        return BBox<T,S>(*this).expand(f);
    }
    
    BBox<T,S>& translate(const Vec<T,S>& delta) {
        min += delta;
        max += delta;
        return *this;
    }
    
    const BBox<T,S> translated(const Vec<T,S>& delta) const {
        return BBox<T,S>(*this).translate(delta);
    }
};

template <typename T, class Op>
void eachBBoxFace(const BBox<T,3>& bbox, Op& op) {
    const Vec<T,3> size = bbox.size();
    const Vec<T,3> x(size.x(), static_cast<T>(0.0), static_cast<T>(0.0));
    const Vec<T,3> y(static_cast<T>(0.0), size.y(), static_cast<T>(0.0));
    const Vec<T,3> z(static_cast<T>(0.0), static_cast<T>(0.0), size.z());
    
    op(bbox.max, bbox.max - y, bbox.max - y - x, bbox.max - x); // top
    op(bbox.min, bbox.min + x, bbox.min + x + y, bbox.min + y); // bottom
    op(bbox.min, bbox.min + z, bbox.min + z + x, bbox.min + x); // front
    op(bbox.max, bbox.max - x, bbox.max - x - z, bbox.max - z); // back
    op(bbox.min, bbox.min + y, bbox.min + y + z, bbox.min + z); // left
    op(bbox.max, bbox.max - z, bbox.max - z - y, bbox.max - y); // right
}

template <typename T, class Op>
void eachBBoxEdge(const BBox<T,3>& bbox, Op& op) {
    const Vec<T,3> size = bbox.size();
    const Vec<T,3> x(size.x(), static_cast<T>(0.0), static_cast<T>(0.0));
    const Vec<T,3> y(static_cast<T>(0.0), size.y(), static_cast<T>(0.0));
    const Vec<T,3> z(static_cast<T>(0.0), static_cast<T>(0.0), size.z());
    
    Vec<T,3> v1, v2;
    
    // top edges clockwise (viewed from above)
    op(bbox.max,         bbox.max - y    );
    op(bbox.max - y,     bbox.max - y - x);
    op(bbox.max - y - x, bbox.max - x    );
    op(bbox.max - x,     bbox.max        );
    
    // bottom edges clockwise (viewed from below)
    op(bbox.min,         bbox.min + x    );
    op(bbox.min + x,     bbox.min + x + y);
    op(bbox.min + x + y, bbox.min + y    );
    op(bbox.min + y,     bbox.min        );
    
    // side edges clockwise (viewed from above)
    op(bbox.min,         bbox.min + z        );
    op(bbox.min + y,     bbox.min + y + z    );
    op(bbox.min + x + y, bbox.min + x + y + z);
    op(bbox.min + x,     bbox.min + x + z    );
}

template <typename T>
typename Vec<T,3>::List bBoxVertices(const BBox<T,3>& bbox) {
    const Vec<T,3> size = bbox.size();
    const Vec<T,3> x(size.x(), static_cast<T>(0.0), static_cast<T>(0.0));
    const Vec<T,3> y(static_cast<T>(0.0), size.y(), static_cast<T>(0.0));
    const Vec<T,3> z(static_cast<T>(0.0), static_cast<T>(0.0), size.z());

    typename Vec<T,3>::List vertices(8);
    
    // top vertices clockwise (viewed from above)
    vertices[0] = bbox.max;
    vertices[1] = bbox.max-y;
    vertices[2] = bbox.min+z;
    vertices[3] = bbox.max-x;
    
    // bottom vertices clockwise (viewed from below)
    vertices[4] = bbox.min;
    vertices[5] = bbox.min+x;
    vertices[6] = bbox.max-z;
    vertices[7] = bbox.min+y;
    return vertices;
}

template <typename T, class Op>
void eachBBoxVertex(const BBox<T,3>& bbox, Op& op) {
    const Vec<T,3> size = bbox.size();
    const Vec<T,3> x(size.x(), static_cast<T>(0.0), static_cast<T>(0.0));
    const Vec<T,3> y(static_cast<T>(0.0), size.y(), static_cast<T>(0.0));
    const Vec<T,3> z(static_cast<T>(0.0), static_cast<T>(0.0), size.z());
    
    // top vertices clockwise (viewed from above)
    op(bbox.max);
    op(bbox.max-y);
    op(bbox.min+z);
    op(bbox.max-x);
    
    // bottom vertices clockwise (viewed from below)
    op(bbox.min);
    op(bbox.min+x);
    op(bbox.max-z);
    op(bbox.min+y);
}

template <typename T>
struct RotateBBox {
    Quat<T> rotation;
    bool first;
    BBox<T,3> bbox;
    
    RotateBBox(const Quat<T>& i_rotation) :
    rotation(i_rotation),
    first(true) {}
    
    void operator()(const Vec<T,3>& vertex) {
        if (first) {
            bbox.min = bbox.max = rotation * vertex;
            first = false;
        } else {
            bbox.mergeWith(rotation * vertex);
        }
    }
};

template <typename T>
BBox<T,3> rotateBBox(const BBox<T,3> bbox, const Quat<T>& rotation, const Vec<T,3>& center = Vec<T,3>::Null) {
    RotateBBox<T> rotator(rotation);
    eachBBoxVertex(bbox.translated(-center), rotator);
    return rotator.bbox.translated(center);
}

template <typename T>
struct TransformBBox {
    Mat<T,4,4> transformation;
    bool first;
    BBox<T,3> bbox;
    
    TransformBBox(const Mat<T,4,4>& i_transformation) :
    transformation(i_transformation),
    first(true) {}
    
    void operator()(const Vec<T,3>& vertex) {
        if (first) {
            bbox.min = bbox.max = transformation * vertex;
            first = false;
        } else {
            bbox.mergeWith(transformation * vertex);
        }
    }
};

template <typename T>
BBox<T,3> rotateBBox(const BBox<T,3> bbox, const Mat<T,4,4>& transformation) {
    TransformBBox<T> transformator(transformation);
    eachBBoxVertex(bbox, transformator);
    return transformator.bbox;
}


typedef BBox<float,3> BBox3f;
typedef BBox<double,3> BBox3d;

#endif
