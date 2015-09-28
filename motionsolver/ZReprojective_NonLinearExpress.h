// ZReprojective_NonLinearExpress.h: interface for the ZReprojective_NonLinearExpress class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ZREPROJECTIVE_NONLINEAREXPRESS_H__4144AEEA_5AF3_4FA8_A4F2_BE2E47D1F8F0__INCLUDED_)
#define AFX_ZREPROJECTIVE_NONLINEAREXPRESS_H__4144AEEA_5AF3_4FA8_A4F2_BE2E47D1F8F0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ZAnyScaleFunction.h"
#include "math/basic/WmlMath/include/WmlMatrix4.h"
#include "math/basic/WmlMath/include/Wmlvector2.h"

class ZReprojective_NonLinearExpress  : public ZAnyScaleFunction<double>
{
public:
	ZReprojective_NonLinearExpress();
	virtual ~ZReprojective_NonLinearExpress();

	virtual double Value (const Wml::GVectord& x);
	
	virtual void Gradient (const Wml::GVectord& x, Wml::GVectord& grad);
	
	virtual void Hesse (const Wml::GVectord& x, Wml::GMatrixd& hesse);

	void SetXVar(const Wml::GVectord& x);

	int GetPointNumber(){return m_points.size();}	

	void SetP(Wml::Matrix4d P);


public:	
	double X,Y,Z;
	double u1,v1;
	double P00, P01, P02, P10, P11, P12, P20, P21, P22, P03, P13, P23;
	double w;
	std::vector<int> m_varIndex;
	std::vector<double> wList;
	std::vector<Wml::Vector2d>	m_points;
	std::vector<Wml::Matrix4d> PList;
};

#endif // !defined(AFX_ZREPROJECTIVE_NONLINEAREXPRESS_H__4144AEEA_5AF3_4FA8_A4F2_BE2E47D1F8F0__INCLUDED_)
