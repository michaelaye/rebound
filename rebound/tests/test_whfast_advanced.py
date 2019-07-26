import rebound
import unittest
import math
import rebound.data
import warnings
from future.utils import with_metaclass
    
whfastsettings = [ # corrector, corrector2, kernel, relative error
        [0, 0,"default",2e-7], 
        [3, 0,"default",1e-9], 
        [5, 0,"default",1e-9], 
        [7, 0,"default",1e-9], 
        [11,0,"default",1e-9], 
        [17,0,"default",1e-9], 
        [0, 0,"modifiedkick",2e-7], 
        [3, 0,"modifiedkick",1e-9], 
        [5, 0,"modifiedkick",1e-11], 
        [7, 0,"modifiedkick",1e-12], 
        [11,0,"modifiedkick",1e-12], 
        [17,0,"modifiedkick",1e-12], 
        [17,0,"composition",1e-12], 
        [17,0,"lazy",1e-12], 
        [11,1,"modifiedkick",1e-12], 
        ]

whfastsettings2 = [ # corrector, corrector2, kernel, relative error
        [0, 0,"default",2e-7], 
        [3, 0,"default",1e-9], 
        [3, 0,"modifiedkick",1e-9], 
        [3, 0,"composition",1e-12], 
        [3, 0,"lazy",1e-12], 
        [3, 0,"default",1e-9], 
        [3, 0,"modifiedkick",1e-9], 
        [3, 0,"composition",1e-12], 
        [3, 0,"lazy",1e-12], 
        [3, 1,"lazy",1e-12], 
        ]

class TestSequenceMetaWHFastAdvanced(type):
    def __new__(mcs, name, bases, dct):

        def energy(s):
            def test(self):
                corrector, corrector2, kernel, maxerror = s
                sim = rebound.Simulation()
                rebound.data.add_outer_solar_system(sim)
                sim.integrator = "whfast"
                sim.ri_whfast.corrector = corrector 
                sim.ri_whfast.corrector2 = corrector2
                sim.ri_whfast.kernel = kernel
                sim.ri_whfast.safe_mode = False
                sim.dt = 0.0123235235*sim.particles[1].P  
                e0 = sim.calculate_energy()
                sim.integrate(1000.*2.*3.1415)
                e1 = sim.calculate_energy()
                self.assertLess(math.fabs((e0-e1)/e1),maxerror)
            return test
        
        def energy_notcom(s):
            def test(self):
                corrector, corrector2, kernel, maxerror = s
                sim = rebound.Simulation()
                rebound.data.add_outer_solar_system(sim)
                for p in sim.particles:
                    p.vx += 1.
                com = sim.calculate_com()
                sim.integrator = "whfast"
                sim.ri_whfast.corrector = corrector 
                sim.ri_whfast.corrector2 = corrector2
                sim.ri_whfast.kernel = kernel
                sim.dt = 0.0123235235*sim.particles[1].P  
                e0 = sim.calculate_energy()
                sim.integrate(1000.*2.*3.1415)
                e1 = sim.calculate_energy()
                self.assertLess(math.fabs((e0-e1)/e1),maxerror)
                com1 = sim.calculate_com()
                self.assertLess(math.fabs((com.x+com.vx*sim.t-com1.x)/(com1.x+com1.y)),1e-12)
                self.assertLess(math.fabs((com.y+com.vy*sim.t-com1.y)/(com1.x+com1.y)),1e-12)
            return test
        
        def compias(s):
            def test(self):
                corrector, corrector2, kernel, maxerror = s
                sim = rebound.Simulation()
                rebound.data.add_outer_solar_system(sim)
                for p in sim.particles:
                    p.vx += 1050. # move out of com to make it harder
                sim.integrator = "whfast"
                sim.ri_whfast.corrector = corrector 
                sim.ri_whfast.corrector2 = corrector2
                sim.ri_whfast.kernel = kernel
                sim.dt = 0.0123235235*sim.particles[1].P  
                sim.integrate(13.21415,exact_finish_time=False)
                
                simi = rebound.Simulation()
                simi.integrator = "ias15"
                rebound.data.add_outer_solar_system(simi)
                for p in simi.particles:
                    p.vx += 1050.
                simi.integrate(sim.t,exact_finish_time=True)
                
                for i in range(sim.N):
                    if corrector:
                        self.assertLess(math.fabs(simi.particles[i].x-sim.particles[i].x),1e-8)
                    else: # less accurate
                       self.assertLess(math.fabs(simi.particles[i].x-sim.particles[i].x),2e-7)
            return test
        
        def backandforth(s):
            def test(self):
                corrector, corrector2, kernel, maxerror = s
                sim = rebound.Simulation()
                rebound.data.add_outer_solar_system(sim)
                for p in sim.particles:
                    p.vx += 1.
                sim0=sim.copy()
                sim.integrator = "whfast"
                sim.ri_whfast.corrector = corrector 
                sim.ri_whfast.corrector2 = corrector2
                sim.ri_whfast.kernel = kernel
                sim.dt = 0.0123235235*sim.particles[1].P  
                steps = 10
                for i in range(steps):
                    sim.step()
                sim.dt *= -1
                for i in range(steps):
                    sim.step()
                for i in range(sim.N):
                    if corrector:
                        self.assertLess(math.fabs(sim0.particles[i].x-sim.particles[i].x),2e-11)
            return test
        
        for s in whfastsettings2:
            test_name = "test_backandforth_WHFastAdvanced_c_%02d_c2_%d_kernel_%s" % (s[0], s[1], s[2])
            dct[test_name] = backandforth(s)

        for s in whfastsettings:
            test_name = "test_energy_WHFastAdvanced_c_%02d_c2_%d_kernel_%s" % (s[0], s[1], s[2])
            dct[test_name] = energy(s)
            test_name = "test_energy_notcom_WHFastAdvanced_c_%02d_c2_%d_kernel_%s" % (s[0], s[1], s[2])
            dct[test_name] = energy_notcom(s)
            test_name = "test_compias_WHFastAdvanced_c_%02d_c2_%d_kernel_%s" % (s[0], s[1], s[2])
            dct[test_name] = compias(s)
        return type.__new__(mcs, name, bases, dct)

class TestSequenceWHFastAdvanced(with_metaclass(TestSequenceMetaWHFastAdvanced, unittest.TestCase)):
    pass


if __name__ == "__main__":
    unittest.main()