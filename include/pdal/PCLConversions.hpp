/******************************************************************************
* Copyright (c) 2012-2013, Bradley J Chambers (brad.chambers@gmail.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
*
* Inspired, and partially borrowed from VTK_PCL_Conversions
* https://github.com/daviddoria/VTK_PCL_Conversions
****************************************************************************/

#pragma once

#include <pdal/PointBuffer.hpp>

#ifdef PDAL_HAVE_PCL
#include <pcl/io/pcd_io.h>
#include <pcl/for_each_type.h>
#include <pcl/point_types.h>
#include <pcl/point_traits.h>
#endif

namespace
{

using namespace pdal;

template<typename CLOUDFETCH>
void setValues(PointBuffer& buf, Dimension *dim, size_t numPts,
    CLOUDFETCH fetcher)
{
    switch (dim->getInterpretation())
    {
        case dimension::Float:
        case dimension::UnsignedInteger:
        case dimension::SignedInteger:
            for (size_t i = 0; i < numPts; ++i)
                buf.setField(*dim, i, fetcher(i) / dim->getNumericScale());
            break;
        case dimension::RawByte:
        case dimension::Pointer:
        case dimension::Undefined:
            throw pdal_error("Dimension data type unable to be scaled "
                    "in conversion from PCL to PDAL");
    }
}

} //namespace

namespace pdal
{
/**
 * \brief Convert PCD point cloud to PDAL.
 *
 * Converts PCD data to PDAL format.
 */
template <typename CloudT>
void PCDtoPDAL(CloudT &cloud, PointBuffer& buf, Dimension *xDim,
    Dimension *yDim, Dimension *zDim, Dimension *iDim)
{
#ifdef PDAL_HAVE_PCL
    typedef typename pcl::traits::fieldList<typename CloudT::PointType>::type
        FieldList;

    if (pcl::traits::has_xyz<typename CloudT::PointType>::value)
    {
        auto getX = [&cloud](size_t i)
            { return cloud.points[i].x; };
        auto getY = [&cloud](size_t i)
            { return cloud.points[i].y; };
        auto getZ = [&cloud](size_t i)
            { return cloud.points[i].z; };
        setValues(buf, xDim, cloud.points.size(), getX);
        setValues(buf, yDim, cloud.points.size(), getY);
        setValues(buf, zDim, cloud.points.size(), getZ);
    }

    if (pcl::traits::has_intensity<typename CloudT::PointType>::value && iDim)
    {
        for (size_t i = 0; i < cloud.points.size(); ++i)
        {
            float f;
            bool hasIntensity = true;

            typename CloudT::PointType p = cloud.points[i];
            pcl::for_each_type<FieldList>
                (pcl::CopyIfFieldExists<typename CloudT::PointType, float>
                    (p, "intensity", hasIntensity, f));
            buf.setField(*iDim, i, f);
        }
    }
#endif
}


/**
 * \brief Convert PDAL point cloud to PCD.
 *
 * Converts PDAL data to PCD format.
 */
template <typename CloudT>
void PDALtoPCD(PointBuffer& data, CloudT &cloud, Dimension *xDim,
    Dimension *yDim, Dimension *zDim, Dimension *iDim)
{
#ifdef PDAL_HAVE_PCL
    typedef typename pcl::traits::fieldList<typename CloudT::PointType>::type
        FieldList;

    cloud.width = data.size();
    cloud.height = 1;  // unorganized point cloud
    cloud.is_dense = false;
    cloud.points.resize(cloud.width);

    if (pcl::traits::has_xyz<typename CloudT::PointType>::value)
    {
        for (size_t i = 0; i < cloud.points.size(); ++i)
        {
            double xd = data.getFieldAs<int32_t>(*xDim, i, false) *
                xDim->getNumericScale();
            double yd = data.getFieldAs<int32_t>(*yDim, i, false) *
                yDim->getNumericScale();
            double zd = data.getFieldAs<double>(*zDim, i, false) *
                zDim->getNumericScale();

            typename CloudT::PointType p = cloud.points[i];
            p.x = (float)xd;
            p.y = (float)yd;
            p.z = (float)zd;
            cloud.points[i] = p;
        }
    }

    if (pcl::traits::has_intensity<typename CloudT::PointType>::value && iDim)
    {
        for (size_t i = 0; i < cloud.points.size(); ++i)
        {
            float f = data.getFieldAs<float>(*iDim, i);

            typename CloudT::PointType p = cloud.points[i];
            pcl::for_each_type<FieldList>
                (pcl::SetIfFieldExists<typename CloudT::PointType, float>
                    (p, "intensity", f));
            cloud.points[i] = p;
        }
    }
#endif //
}

}  // namespace pdal
