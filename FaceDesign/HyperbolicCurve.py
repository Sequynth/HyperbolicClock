# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

import numpy as np
import matplotlib.pyplot as plt

## physical parameters of the clock
# max radius for calculations
rMax    = 80;#mm
# distance of hour hand from axis
r       = 20;#mm
# angle of hour hand relative to xy-plane
phi     = 45 / 180*np.pi;

## values derived from physical parameters
# max z for calculation
zMax    = rMax * np.tan(phi);
# length of hand
l       = 2*zMax / np.sin(phi);
# projected into xy-plane
l_xy    = l * np.cos(phi);
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
alpha       = np.arange(degSteps, 360, degSteps) / 180*np.pi;
# steps per hour
sph     = 360/degSteps / 12;
print('Steps per hour: %.1f at %.3f steps' % (sph, degSteps));
print('Angle per hour: {0}'.format(360/12));
# height as a function of alpha
hAlpha = np.zeros_like(alpha);
hAlpha =  r * np.tan(phi)/np.tan(alpha);

# calculate angle during which the hand does not protrude the Face
alphaNP = 2* np.arctan(r / (l_xy/2)) / np.pi*180;
print('Angle of no protrusion: %.2f' % alphaNP);

# get the indices for the angles, that are protruded between -zMax and zMax
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
dotNumber   = np.roll(np.arange(12, 0, -1), 8);
print(dotHeights)
print(dotAngles / np.pi*180)

# show results
plt.figure()
plt.plot(x1, h)
plt.plot(x2, h)
plt.plot(dotRadii, dotHeights, 'r+')

for ii in range(0, 12, 1):
    plt.text(dotRadii[ii], dotHeights[ii], str(dotNumber[ii]));

#plt.plot([-rMax, rMax], [-zMax, zMax])
plt.axis('equal')
#plt.savefig("Hyperbola.svg", format="svg")

#plt.figure()
#plt.plot(alpha, hAlpha)
#plt.plot()
#plt.ylim(-zMax, zMax)

plt.show()
