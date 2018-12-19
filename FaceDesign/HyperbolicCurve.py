# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

import numpy as np
import matplotlib.pyplot as plt

# max radius for calculations
rMax    =  80;#mm
# distance of hour hand from axis
r       =  20;#mm

# angle of hour hand relative to xy-plane
phi     = 45 / 180*np.pi;

# max z for calculation
zMax    = rMax * np.tan(phi);

# plot range in z-direction
h       = np.linspace(-zMax, zMax, 100);

# calculate the trajectory by calculating for each height h, the distance of the
# hand from the rotation axis. h/tan(phi) is the projection of the hand to the
# xy-plane, in which x1 and x2 are calculated.
x1      =  np.sqrt(np.power(r, 2) + np.power(h/np.tan(phi), 2));
x2      = -np.sqrt(np.power(r, 2) + np.power(h/np.tan(phi), 2));

# conversion from height to rotation angle alpha (considering only positive x-direction)
alphaPlus   = np.arctan2(r, (h/np.tan(phi)) ) / np.pi*180;
# considering also negative x
alphaMinus  = -alphaPlus

# equidistant steps of alpha in rad
degSteps    = 0.25;
alpha       = np.arange(degSteps, 180, degSteps) / 180*np.pi;
# steps per hour
sph     = 360/degSteps / 12;
print('steps per hour: {0}'.format(sph));
# height as a function of alpha
hAlpha  = r * np.tan(phi)/np.tan(alpha)

# calculate angle during which the hand does not potrude the Face



# get the indices for the angles, that are potruded between -zMax and zMax
indices = np.where((hAlpha <=zMax) & (hAlpha >= -zMax));
# start with half the steps it takes for one hour
iMin = sph/2;
iMax = np.max(indices);
print('Minimum: {0}'.format(iMin))
print('Maximum: {0}'.format(iMax))

# calculate radius and height for possible hour locations
dotAngles   = degSteps * np.arange(iMin, iMax, sph) / 180*np.pi;
dotRadii    = r / np.sin(dotAngles);
dotHeights  = r * np.tan(phi)/np.tan(dotAngles)

print(dotRadii)

# angle of asymptote to y axis
gamma = np.arctan(2/np.cos(phi))

x = np.linspace(-rMax, rMax, 2);

plt.figure()
plt.plot(x1, h)
plt.plot(x2, h)
plt.plot(alphaPlus, h)
plt.plot(alphaMinus, h)
plt.plot(dotRadii, dotHeights, 'r+')
plt.plot([-rMax, rMax], [-zMax, zMax])
plt.axis('equal')
#plt.savefig("Hyperbola.svg", format="svg")
#plt.xlim(-R, R)
#plt.ylim(-R, R)

plt.figure()
plt.plot(alpha, hAlpha)
plt.plot()
plt.ylim(-zMax, zMax)
# plt.axis('equal')

plt.show()
