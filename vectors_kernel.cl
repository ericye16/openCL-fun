#define CHANGE_X 1
#define CHANGE_Z 2
#define CHANGE_Y 4

__constant int xSize = 32;
__constant int ySize = 32;
__constant int zSize = 32;
__constant float d = 1.f;
__constant float dt = 0.01f;
__constant float xMax = 40;
__constant float zMax = 40;

int lin(int x, int y, int z);
float poly6(float r);
float poly6_grad(float r);
float poly6_laplace(float r);
int boundaries(float3 position);

__kernel 
void fluids(
    __global float3 * initialVelocity, //0
    __global float3 * initialPosition, //1
    __constant float * mass, //2
    __global float * density, //3
    __global float * pressure, //4
    __constant float * gasConstant, //5
    __constant float * mu, //6
    __constant float3 * gravity, //7
    __global float3 * finalVelocity, //8
    __global float3 * finalPosition) //9
{
    int xIdx = get_global_id(0);
    int yIdx = get_global_id(1);
    int zIdx = get_global_id(2);
    int me = lin(xIdx, yIdx, zIdx);
    __private float3 currentPosition = initialPosition[lin(xIdx, yIdx, zIdx)];
    __private float3 currentVelocity = initialVelocity[lin(xIdx, yIdx, zIdx)];
    
    __private float3 nextVelocity;
    __private float3 nextPosition;
    __private float currentPressure = 0;
    __private float3 f_pressure = {0, 0, 0};
    __private float3 f_viscosity;
    __private float our_mu = * mu;
    
    /*let's find the density.
    rho(x) = sum_j (m_j W(abs(x - x_j)))
    it's the sum of all the nearby masses multiplied by the smoothing function.
    */
    __private float currentLocalDensity;
    currentLocalDensity = 0;
    for (int i = 0; i < xSize * ySize * zSize; i++) {
        currentLocalDensity += mass[i] * poly6(fabs(fast_distance(currentPosition, initialPosition[i])));
    }
    density[me] = currentLocalDensity;
    barrier(CLK_GLOBAL_MEM_FENCE);
    
    currentPressure = *gasConstant * currentLocalDensity;
    pressure[me] = currentPressure;
    barrier(CLK_GLOBAL_MEM_FENCE);
    
    __private float their_mass;
    __private float3 their_position;
    __private float their_pressure;
    __private float their_density;
    __private float3 their_velocity;
    for (int i = 0; i < xSize * ySize * zSize; i++) {
        their_mass = mass[i];
        their_position = initialPosition[i];
        their_pressure = pressure[i];
        f_pressure.x -= their_mass * (currentPressure + their_pressure) / (2 * their_density) * poly6_grad(
            fabs(currentPosition.x - their_position.x)
        );
        f_pressure.y -= their_mass * (currentPressure + their_pressure) / (2 * their_density) * poly6_grad(
            fabs(currentPosition.y - their_position.y)
        );
        f_pressure.z -= their_mass * (currentPressure + their_pressure) / (2 * their_density) * poly6_grad(
            fabs(currentPosition.z - their_position.z)
        );
        
        
        f_viscosity.x += their_mass * (their_velocity.x - currentVelocity.x) / their_density * poly6_laplace(
            fabs(currentPosition.x - their_position.x)
        );
        f_viscosity.y += their_mass * (their_velocity.y - currentVelocity.y) / their_density * poly6_laplace(
            fabs(currentPosition.y - their_position.y)
        );
        f_viscosity.z += their_mass * (their_velocity.z - currentVelocity.z) / their_density * poly6_laplace(
            fabs(currentPosition.z - their_position.z)
        );
    }
    f_viscosity *= our_mu;
    
    __private float3 f_gravity;
    f_gravity = *gravity * currentLocalDensity;
    
    //combine the forces and stuff
    __private float3 f_total;
    f_total = f_pressure + f_gravity + f_viscosity;
    nextVelocity = currentVelocity + f_total / currentLocalDensity * dt;
    
    nextPosition = currentPosition + nextVelocity * dt;
    
    __private int boundaryChange;
    boundaryChange = boundaries(nextPosition);
    if (boundaryChange & CHANGE_X) {
        nextVelocity.x *= -1;
    }
    if (boundaryChange & CHANGE_Y) {
        nextVelocity.y *= -1;
    }
    if (boundaryChange & CHANGE_Z) {
        nextVelocity.z *= -1;
    }
    
    finalVelocity[me] = nextVelocity;
    finalPosition[me] = nextPosition;    
}

int lin(int x, int y, int z) {
    return z * xSize * ySize + xSize * y + x;
}

float poly6(float r) {
    if (r >= 0 && r <= d) {
        return 315 * pow(pow(d, 2) - pow(r, 2), 3) /(64 * M_PI_F * pow(d, 9));
    }
    else return 0;
}

float poly6_grad(float r) {
    if (r >= 0 && r <= d) {
        return -945 / (32 * M_PI_F * pow(d, 9)) * r * pow(pow(d, 2) - pow(d, 2), 2);
    }
    else {
        return 0;
    }
}

float poly6_laplace(float r) {
    if (r >= 0 && r <= d) {
        return -945 / (32 * M_PI_F * pow(d, 9)) * (pow(d, 4) - 6 * pow(d, 2) * pow(r, 2) + 5 * pow(r, 4));
    }
    else {
        return 0;
    }
}

int boundaries(float3 position) {
    int toReturn = 0;
    if (position.x > xMax || position.x < 0) {
        toReturn |= CHANGE_X;
    }
    if (position.y < 0) {
        toReturn |= CHANGE_Y;
    }
    if (position.z > zMax || position.z < 0) {
        toReturn |= CHANGE_Z;
    }
    return toReturn;    
}


