#include "Trail/Spline.hpp"
#include "Trail/SplineControlPoint.hpp"

using namespace Sombrero;

Spline::Spline() { Reserve(10); }
Spline::Spline(int preCount)
{
    Reserve(preCount);
}

void Spline::Reserve(int count)
{
    if (controlPoints.size() < count)
        controlPoints.reserve(count);
    if (segments.size() < count)
        segments.reserve(count);
}

SplineControlPoint* Spline::operator[](int index)
{
    if (index > -1 && index < segments.size())
        return segments[index].get();
    return nullptr;
}

std::vector<std::shared_ptr<SplineControlPoint>>& Spline::get_ControlPoints()
{
    return controlPoints;
}

SplineControlPoint* Spline::NextControlPoint(SplineControlPoint* controlPoint)
{
    if (controlPoints.size() == 0)
        return nullptr;

    int i = controlPoint->ControlPointIndex + 1;
    if (i >= controlPoints.size())
        return nullptr;
    return controlPoints.at(i).get();
}

SplineControlPoint* Spline::PreviousControlPoint(SplineControlPoint* controlPoint)
{
    if (controlPoints.size() == 0)
        return nullptr;

    int i = controlPoint->ControlPointIndex - 1;
    if (i < 0)
        return nullptr;
    return controlPoints.at(i).get();
}

FastVector3 Spline::NextPosition(SplineControlPoint* controlPoint)
{
    int i = controlPoint->ControlPointIndex + 1;
    if (i >= controlPoints.size())
        return controlPoint->Position;
    return controlPoints.at(i).get()->Position;
}

FastVector3 Spline::PreviousPosition(SplineControlPoint* controlPoint)
{
    int i = controlPoint->ControlPointIndex - 1;
    if (i < 0)
        return controlPoint->Position;
    return controlPoints.at(i).get()->Position;
}

FastVector3 Spline::NextNormal(SplineControlPoint* controlPoint)
{
    int i = controlPoint->ControlPointIndex + 1;
    if (i >= controlPoints.size())
        return controlPoint->Normal;
    return controlPoints.at(i).get()->Normal;
}

FastVector3 Spline::PreviousNormal(SplineControlPoint* controlPoint)
{
    int i = controlPoint->ControlPointIndex - 1;
    if (i < 0)
        return controlPoint->Normal;
    return controlPoints.at(i).get()->Normal;
}

SplineControlPoint* Spline::LenToSegment(float t, float& localF)
{
    // init with first segment to prevent crashes, was inited with nullptr
    SplineControlPoint* seg = nullptr;

    float len = Sombrero::Clamp01(t) * segments.back()->Dist;

    int index = 0;
    for (auto test : segments)
    {
        if (test->Dist >= len)
        {
            seg = test.get();
            break;
        }
        index++;
    }

    if (!index)
    {
        localF = 0.0f;
        return seg;
    }

    int prevIdx = seg->SegmentIndex - 1;
    auto prevSeg = segments.at(prevIdx);
    auto prevLen = seg->Dist - prevSeg->Dist;
    localF = (len - prevSeg->Dist) / prevLen;
    return prevSeg.get();
}

FastVector3 Spline::CatmulRom(const FastVector3& T0, const FastVector3& P0, const FastVector3& P1, const FastVector3& T1, float f)
{
    static constexpr const double DT1 = -0.5;
    static constexpr const double DT2 = 1.5;
    static constexpr const double DT3 = -1.5;
    static constexpr const double DT4 = 0.5;

    static constexpr const double DE2 = -2.5;
    static constexpr const double DE3 = 2;
    static constexpr const double DE4 = -0.5;

    static constexpr const double DV1 = -0.5;
    static constexpr const double DV3 = 0.5;

    double FAX = DT1 * T0.x + DT2 * P0.x + DT3 * P1.x + DT4 * T1.x;
    double FBX = T0.x + DE2 * P0.x + DE3 * P1.x + DE4 * T1.x;
    double FCX = DV1 * T0.x + DV3 * P1.x;
    double FDX = P0.x;

    double FAY = DT1 * T0.y + DT2 * P0.y + DT3 * P1.y + DT4 * T1.y;
    double FBY = T0.y + DE2 * P0.y + DE3 * P1.y + DE4 * T1.y;
    double FCY = DV1 * T0.y + DV3 * P1.y;
    double FDY = P0.y;

    double FAZ = DT1 * T0.z + DT2 * P0.z + DT3 * P1.z + DT4 * T1.z;
    double FBZ = T0.z + DE2 * P0.z + DE3 * P1.z + DE4 * T1.z;
    double FCZ = DV1 * T0.z + DV3 * P1.z;
    double FDZ = P0.z;

    float FX = (float)(((FAX * f + FBX) * f + FCX) * f + FDX);
    float FY = (float)(((FAY * f + FBY) * f + FCY) * f + FDY);
    float FZ = (float)(((FAZ * f + FBZ) * f + FCZ) * f + FDZ);

    return FastVector3(FX, FY, FZ);
}

FastVector3 Spline::InterpolateByLen(float tl)
{
    float localF;
    SplineControlPoint* seg = LenToSegment(tl, localF);
    if (!seg)
        return FastVector3::zero();
    return seg->Interpolate(localF);
}

FastVector3 Spline::InterpolateNormalByLen(float tl)
{
    float localF;
    SplineControlPoint* seg = LenToSegment(tl, localF);
    if (!seg)
        return FastVector3::up();
    return seg->InterpolateNormal(localF);
}

SplineControlPoint* Spline::AddControlPoint(const FastVector3& pos, const FastVector3& up)
{
    std::shared_ptr<SplineControlPoint> cp = std::make_shared<SplineControlPoint>();
    cp->Init(this);
    cp->Position = pos;
    cp->Normal = up;
    controlPoints.push_back(cp);
    cp->ControlPointIndex = controlPoints.size() - 1;
    return cp.get();
}

void Spline::Clear()
{
    controlPoints.clear();
    segments.clear();
}

void Spline::RefreshDistance()
{
    if (segments.size() < 1)
        return;

    segments.at(0)->Dist = 0.0f;

    for (int i = 1; i < segments.size(); i++)
    {
        float prevLen = (segments.at(i)->Position - segments.at(i - 1)->Position).Magnitude();
        segments[i]->Dist = segments[i - 1]->Dist + prevLen;
    }
}

void Spline::RefreshSpline()
{
    segments.clear();
    int size = 0;
    for (auto point : controlPoints)
    {
        if (point->IsValid())
        {
            segments.push_back(point);
            point->SegmentIndex = size++;
        }
    }

    RefreshDistance();
}

Spline::~Spline()
{
    Clear();
}