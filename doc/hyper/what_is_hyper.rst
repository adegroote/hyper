What is Hyper ?
===============

**hyper** is a tool to develop supervision layer for complex robot autonomous
system. In other words, it is a tool which helps you describe how to
coordinate in an intelligent way the different underlying processes of your
functional layer. 

It is an implementation of the **ROAR** architecture, which is described in
several academic papers. See them for a general understanding of the proposed
architecture:

- `ICRA 2011 paper <http://homepages.laas.fr/adegroot/publis/ICRA2011_ROAR.pdf>`_
- `a journal paper (under review) <http://homepages.laas.fr/adegroot/publis/IJRR_2011.pdf>`_
- `my thesis (french only) <http://homepages.laas.fr/adegroot/thesis.pdf>`_

In a few words, each kind of *resource* (which can be an hardware resource, a
piece of information about past, present of even future), is encapsulated in
its own agent. These agents can exchange their states and requests some
constraint to other agents. In response, involved agents try to fulfill their
requests, by starting some specific behaviours or inform the caller that it is
not possible (depending its internal state). Several mechanisms enables
cooperation between the different agents to have a globally consistent
behaviour.
