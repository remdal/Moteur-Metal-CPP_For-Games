/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                        +       +          */
/*      File: RMDLMathUtils.hpp            +++     +++		**/
/*                                        +       +          */
/*      By: Laboitederemdal      **        +       +        **/
/*                                       +           +       */
/*      Created: 19/09/2025 20:09:59      + + + + + +   * ****/
/*                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MathUtils_hpp
# define MathUtils_hpp

# include <simd/simd.h>

namespace math
{
    simd::float3 add( const simd::float3& a, const simd::float3& b );
    simd::float4x4 makeIdentity();
    simd::float4x4 makeOrtho(float left, float right, float top, float bottom, float near, float far);
    simd::float4x4 makePerspective( float fovRadians, float aspect, float znear, float zfar );
    simd::float4x4 makeXRotate( float angleRadians );
    simd::float4x4 makeYRotate( float angleRadians );
    simd::float4x4 makeZRotate( float angleRadians );
    simd::float4x4 makeTranslate( const simd::float3& v );
    simd::float4x4 makeScale( const simd::float3& v );
    simd::float4x3 discardTranslation( const simd::float4x4& m );
}

#endif /* MathUtils_hpp */