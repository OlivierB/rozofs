# -*- coding: utf-8 -*-
#
# Copyright (c) 2013 Fizians SAS. <http://www.fizians.com>
# This file is part of Rozofs.
#
# Rozofs is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 2.
#
# Rozofs is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see
# <http://www.gnu.org/licenses/>.

import Pyro.core
import subprocess
import time

from rozofs.core.constants import STORAGED_MANAGER, EXPORTD_MANAGER, \
    AGENT_PORT, LAYOUT_SAFE, EXPORTS_ROOT, \
    STORAGES_ROOT, ROZOFSMOUNT_MANAGER
from rozofs.core.agent import ServiceStatus
from rozofs.core.exportd import VolumeConfig, ClusterConfig, ExportConfig
from rozofs.core.storaged import StorageConfig
from rozofs.core.rozofsmount import RozofsMountConfig
from socket import socket

def get_proxy(host, manager):
    try:
        return Pyro.core.getProxyForURI('PYROLOC://%s:%s/%s' % (host, str(AGENT_PORT), manager))
    except:
        raise Exception("no agent %s reachable for %s" % (manager, host))

class Role(object):
    EXPORTD = 1
    STORAGED = 2
    ROZOFSMOUNT = 4
    ROLES = [1, 2, 4]


class ExportStat(object):
    def __init__(self, eid= -1, vid= -1, bsize= -1, blocks= -1, bfree= -1, files= -1, ffree= -1):
        self.eid = eid
        self.vid = vid
        self.bsize = bsize
        self.blocks = blocks
        self.bfree = bfree
        self.files = files
        self.ffree = ffree

class StorageStat(object):
    def __init__(self, sid= -1, status= -1, size= -1, free= -1):
        self.sid = sid
        self.status = status
        self.size = size
        self.free = free

class VolumeStat(object):
    def __init__(self, vid= -1, bsize= -1, bfree= -1, sstats=[]):
        self.vid = vid
        self.bsize = bsize
        self.bfree = bfree
        self.sstats = sstats

class Node(object):

    def __init__(self, host, roles=0):
        self._host = host
        self._roles = roles
        # for all below key is Role
        self._proxies = {}
        # does our node is connected and running
        self._up = False

    def __del(self):
        self._release_proxies()

    def _initialize_proxies(self):
        try:
            if self.has_roles(Role.EXPORTD) and Role.EXPORTD not in self._proxies:
                self._proxies[Role.EXPORTD] = get_proxy(self._host, EXPORTD_MANAGER)
            if self.has_roles(Role.STORAGED) and Role.STORAGED not in self._proxies:
                self._proxies[Role.STORAGED] = get_proxy(self._host, STORAGED_MANAGER)
            if self.has_roles(Role.ROZOFSMOUNT) and Role.ROZOFSMOUNT not in self._proxies:
                self._proxies[Role.ROZOFSMOUNT] = get_proxy(self._host, ROZOFSMOUNT_MANAGER)
        except Exception as e:
            self._release_proxies()
            raise e

    def _release_proxies(self):
        for p in self._proxies.values():
            p.getProxy()._release()
        self._up = False

    def _try_up(self):
        """ check if a node is up if not try to set it up """
        if self._up == False:
            # is it reachable ?
            with open('/dev/null', 'w') as devnull:
                if subprocess.call(['ping', '-c', '1', self._host], stdout=devnull,
                    stderr=devnull) is not 0:
                        return False
            # yes, so try to connect to agents
            try:
                self._initialize_proxies()
            except:
                return False
            self._up = True

        return True

    def is_up(self):
        return self._up

    def set_roles(self, roles):
        self._roles = roles
        if self._up:
            self._release_proxies()

    def get_roles(self):
        return self._roles

    def has_roles(self, roles):
        return self._roles & roles == roles

    def has_one_of_roles(self, roles):
        for role in [r for r in Role.ROLES if r & roles == r]:
            if self._roles & role == role:
                return True
        return False

    def get_configurations(self, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT):
        if not self._try_up():
            return None

        configurations = {}
        for role in [r for r in Role.ROLES if r & roles == r and self.has_roles(r)]:
            configurations[role] = self._proxies[role].get_service_config()

        return configurations

    def set_configurations(self, configurations):
        if not self._try_up():
            return

        for r, c in configurations.items():
            if self.has_roles(r):
                self._proxies[r].set_service_config(c)

    def get_statuses(self, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT):
        if not self._try_up():
            return None

        statuses = {}
        for role in [r for r in Role.ROLES if r & roles == r and self.has_roles(r)]:
            statuses[role] = self._proxies[role].get_service_status()

        return statuses

    def set_statuses(self, statuses):
        if not self._try_up():
            return
        for r, s in statuses.items():
            if self.has_roles(r):
                self._proxies[r].set_service_status(s)


class Platform(object):
    """ A rozofs platform."""

    def __init__(self, hostname="localhost"):
        """
        Args:
            hostname: platform manager host (should be part of the platform !).
        """
        self._hostname = hostname
        self._nodes = self._get_nodes(hostname)

    def _get_nodes(self, hostname):
        nodes = {}

        exportd_node = Node(hostname, Role.EXPORTD)

        nodes[hostname] = exportd_node

        node_configs = exportd_node.get_configurations(Role.EXPORTD)
        if not node_configs:
            raise Exception("%s unreachable." % hostname)

        econfig = node_configs[Role.EXPORTD]

        if econfig is None:
            raise "exportd node is off line."

        for h in [s for v in econfig.volumes.values()
                        for c in v.clusters.values()
                        for s in c.storages.values()]:
            roles = Role.STORAGED
            # check if has rozofsmount
            try :
                p = get_proxy(h, ROZOFSMOUNT_MANAGER)
                if p.get_service_config():
                    roles = roles | Role.ROZOFSMOUNT
            except:
                pass
            if h in nodes:  # the exportd node !
                nodes[h].set_roles(nodes[h].get_roles() | roles)
            else:
                nodes[h] = Node(h, roles)

        return nodes

    def _check_nodes(self):
        """ Check if all nodes have a platform agent running """
        for n in self._nodes:
            if not n.check_platform_manager():
                raise Exception("%s: check platform failed." % n._host)


    # the first exportd nodes
    def _get_exportd_node(self):
        for n in self._nodes.values():
            if n.has_roles(Role.EXPORTD):
                return n

    def list_nodes(self, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT):
        """ Get all nodes managed by this platform

        Args:
            roles: roles that node should have

        Return:
            A dict: keys are host names, values are roles (as xored flags)
        """
        nodes = {}
        for h, n in self._nodes.items():
            if n.has_one_of_roles(roles):
                nodes[h] = n.get_roles()
        return nodes

    def get_statuses(self, hosts=None, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT):
        """ Get statuses for named nodes and roles

        Args:
            hosts : list of hosts to get statuses from, if None all host are
                    checked

            roles : for which roles statuses should be retrieved
                    if a given host doesn't have this role the return statuses
                    will not contain key for this role (and might be empty).

        Return:
            A dict: keys are host names, values are dicts {Role: ServiceStatus}
                    or None if node is off line
        """
        statuses = {}
        if hosts is None:
            for h, n in self._nodes.items():
                statuses[h] = n.get_statuses(roles)
        else:
            for h in hosts:
                statuses[h] = self._nodes[h].get_statuses(roles)
        return statuses

    def set_statuses(self, statuses):
        """ Set statuses

        Args:
            statuses : A dict where keys are host names,
                       values are dicts {Role: ServiceStatus}

        Warning:
            assuming user knows what he is doing start share before
            exportd or storaged will lead to errors
        """
        for h, s in statuses.items():
            self._nodes[h].set_statuses(s)

    def set_status(self, hosts=None, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT, status=ServiceStatus.STOPPED):
        """ Convenient method to set the same status to hosts.

        Args:
            hosts: list of hosts to set status , if None all host are set

            roles: for which roles status should be set

        Warning:
            see set_statuses
        """
        # while Node set statuses is applied only if a node has this role
        # we just need to call set statuses on all nodes
        statuses_to_set = {}
        for role in [r for r in Role.ROLES if r & roles == r]:
            statuses_to_set[role] = status
        statuses = {}
        if hosts is None:
            for host in self._nodes.keys():
                statuses[host] = statuses_to_set
        else:
            for host in hosts:
                statuses[host] = statuses_to_set

        self.set_statuses(statuses)

    def start(self, hosts=None, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT):
        """ Convenient method to start all nodes with a role
        Args:
            the roles to be started
        """

        # check if hosts are managed
        if hosts is not None:
            for h in hosts:
                if h not in self._nodes.keys():
                    raise Exception('unmanaged host: %s' % h)

        if hosts is None:
            hosts = self._nodes.keys()

        # take care of the starting order
        if roles & Role.STORAGED == Role.STORAGED:
            self.set_status(hosts, Role.STORAGED, ServiceStatus.STARTED)
        if roles & Role.EXPORTD == Role.EXPORTD:
            self.set_status(hosts, Role.EXPORTD, ServiceStatus.STARTED)
        if roles & Role.ROZOFSMOUNT == Role.ROZOFSMOUNT:
            time.sleep(1)
            self.set_status(hosts, Role.ROZOFSMOUNT, ServiceStatus.STARTED)

    def stop(self, hosts=None, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT):
        """ Convenient method to stop all nodes with a role
        Args:
            the roles to be stopped
        """
        if hosts is None:
            hosts = self._nodes.keys()

        # take care of the stopping order
        if roles & Role.ROZOFSMOUNT == Role.ROZOFSMOUNT:
            self.set_status(hosts, Role.ROZOFSMOUNT, ServiceStatus.STOPPED)
        if roles & Role.EXPORTD == Role.EXPORTD:
            self.set_status(hosts, Role.EXPORTD, ServiceStatus.STOPPED)
        if roles & Role.STORAGED == Role.STORAGED:
            self.set_status(hosts, Role.STORAGED, ServiceStatus.STOPPED)

# Not need anymore ?
    def get_configurations(self, hosts=None, roles=Role.EXPORTD | Role.STORAGED | Role.ROZOFSMOUNT):
        """ Get configurations for named nodes and roles

        Args:
            hosts : list of hosts to get statuses from, if None all host are
                    checked

            roles : for which roles statuses should be retrieved
                    if a given host doesn't have this role the return statuses
                    will not contain key for this role.

        Return:
            A dict: keys are host names, values are dicts {Role: configuration}
                    or None if node is off line.
        """
        configurations = {}
        if hosts is None:
            for h, n in self._nodes.items():
                configurations[h] = n.get_configurations(roles)
        else:
            for h in hosts:
                configurations[h] = self._nodes[h].get_configurations(roles)

        return configurations

    # should be done at cli with get/set configuration?!
    def set_layout(self, layout):
        decode = self.decode_layout(layout)

        if not (decode[0] <= decode[1] and decode[1] <= decode[2]) or decode[0] < 1:
            raise Exception("invalid layout: (%i, %i, %i)" % decode)

        node = self._get_exportd_node()
        configuration = node.get_configurations(Role.EXPORTD)

        if configuration is None:
            raise "exportd node is off line."

        if len(configuration[Role.EXPORTD].volumes) != 0:
            raise Exception("platform has configured volume(s) !!!")

        configuration[Role.EXPORTD].layout = layout
        node.set_configurations(configuration)

    def get_layout(self):
        node = self._get_exportd_node()
        configuration = node.get_configurations(Role.EXPORTD)

        if configuration is None:
            raise "exportd node is off line."

        return configuration[Role.EXPORTD].layout

#    def list_volumes(self):
#        node = self._get_exportd_node()
#        configuration = node.get_configurations(Role.EXPORTD)
#
#        if configuration is None:
#            raise "exportd node is off line."
#
#        return configuration[Role.EXPORTD].volumes.keys()
#
#    def get_volume_stats(self, vid):
#        node = self._get_exportd_node()
#        configuration = node.get_configurations(Role.EXPORTD)
#
#        if configuration is None:
#            raise "exportd node is off line."
#
#        if vid not in configuration[Role.EXPORTD].volumes.keys():
#            raise Exception("unmanaged volme: %d" % vid)
#
#        return configuration[Role.EXPORTD].volumes[vid]

    def convert_layout(self, inverse, forward, safe):
        val = 0
        val = val | (inverse & 0xFF)
        val = val | ((forward & 0xFF) << 8)
        val = val | ((safe & 0xFF) << 16)
        return val

    def decode_layout(self, val):
        safe = (val >> 16)
        forward = ((val >> 8) & 0xFF)
        inverse = (val & 0xFF)
        return (inverse, forward, safe)

    def add_nodes(self, hosts, vid=None):
        """ Add storaged nodes to the platform

        Args:
            vid: volume to use, if none a new one will be created
            hosts: hosts to be added
        """
        enode = self._get_exportd_node()
        econfig = enode.get_configurations(Role.EXPORTD)

        if econfig is None:
            raise Exception("exportd node is off line.")

        # should be allowed !!!
        decode = self.decode_layout(econfig[Role.EXPORTD].layout)
        if len(hosts) < decode[LAYOUT_SAFE]:
            raise Exception("too few hosts: %s over %s" % (len(hosts),
                    decode[LAYOUT_SAFE]))

        if vid is not None:
            # check if the given one is a new one else get the related VolumeConfig
            if vid in econfig[Role.EXPORTD].volumes:
                vconfig = econfig[Role.EXPORTD].volumes[vid]
            else:
                vconfig = VolumeConfig(vid)
        else:
            vids = econfig[Role.EXPORTD].volumes.keys()
            if len(vids) != 0 :
                vid = max(vids) + 1
            else:
                vid = 1
            vconfig = VolumeConfig(vid)

        # find a cluster id
        cids = [c.cid for v in econfig[Role.EXPORTD].volumes.values()
                      for c in v.clusters.values()]
        if len(cids) != 0:
            cid = max(cids) + 1
        else:
            cid = 1

        # as we create a new cluster sids always starts at 1
        sid = 1

        cconfig = ClusterConfig(cid)
        roles = Role.STORAGED

        for h in hosts:
            # if it's a new node register it
            # else just update its role
            if h not in self._nodes:
                self._nodes[h] = Node(h, roles)
            else:
                # maybe overkill since the node could already have this role
                self._nodes[h].set_roles(self._nodes[h].get_roles() | roles)

            # configure the storaged
            sconfig = self._nodes[h].get_configurations(Role.STORAGED)
            sconfig[Role.STORAGED].storages[(cid, sid)] = StorageConfig(cid,
                 sid, "%s/storage_%d_%d" % (STORAGES_ROOT, cid, sid))
            self._nodes[h].set_configurations(sconfig)

            # add the storage to the cluster
            cconfig.storages[sid] = h
            sid += 1

        # add the cluster to the volume
        vconfig.clusters[cid] = cconfig
        # update the exportd config
        econfig[Role.EXPORTD].volumes[vid] = vconfig
        enode.set_configurations(econfig)

    def remove_volume(self, vid):
        """ Remove a volume

        Args:
            vid: the volume to be removed
        """
        enode = self._get_exportd_node()
        econfig = enode.get_configurations(Role.EXPORTD)

        if econfig is None:
            raise Exception("Exportd node is off line.")

        if vid not in econfig[Role.EXPORTD].volumes:
            raise Exception("Unknown volume: %d." % vid)

        if [e for e in econfig[Role.EXPORTD].exports.values() if e.vid == vid]:
            raise Exception("Volume has configured export(s)")
        else:
            vconfig = econfig[Role.EXPORTD].volumes.pop(vid)
            for c in vconfig.clusters.values():
                for sid, host in c.storages.items():
                    sconfig = self._nodes[host].get_configurations(Role.STORAGED)
                    sconfig[Role.STORAGED].storages.pop((c.cid, sid))
                    self._nodes[host].set_configurations(sconfig)
            enode.set_configurations(econfig)

    def create_export(self, vid, name=None, passwd=None, squota="", hquota=""):
        """ Export a new file system

        Args:
            vid: the volume id to use the file system relies on
        """
        # exportd_hostname = self.get_exportd_hostname()
        enode = self._get_exportd_node()
        econfig = enode.get_configurations(Role.EXPORTD)

        if econfig is None:
            raise Exception("Exportd node is off line.")

        if vid not in econfig[Role.EXPORTD].volumes:
            raise Exception("Unknown volume: %d." % vid)

        # validate quotas
        if squota:
            if not squota[0].isdigit():
                raise Exception("Invalid squota format: %s." % squota)

            for c in squota:
                if not c.isdigit():
                    if c not in ['K', 'M', 'G']:
                        raise Exception("Invalid squota format: %s." % squota)
                    else:
                        break

        if hquota:
            if not hquota[0].isdigit():
                raise Exception("Invalid hquota format: %s." % hquota)


            for c in hquota:
                if not c.isdigit():
                    if c not in ['K', 'M', 'G']:
                        raise Exception("Invalid hquota format: %s." % squota)
                    else:
                        break

        # find an eid
        eids = econfig[Role.EXPORTD].exports.keys()
        if len(eids) != 0:
            eid = max(eids) + 1
        else:
            eid = 1

        # check name
        if name is None:
            name = "export_%d" % eid
        else:
            if "%s/%s" % (EXPORTS_ROOT, name) in [e.root for e in econfig[Role.EXPORTD].exports.values()]:
                raise Exception("Duplicate export name: %s" % name)

        # compute md5
        if passwd is None:
            md5 = ""
        else:
            with open('/dev/null', 'w') as devnull:
                md5 = subprocess.check_output(['md5pass', passwd, 'rozofs'],
                                   stderr=devnull)[11:].rstrip()

        # add this new export
        econfig[Role.EXPORTD].exports[eid] = ExportConfig(eid, vid, "%s/%s" % (EXPORTS_ROOT, name), md5, squota, hquota)
        enode.set_configurations(econfig)

    def update_export(self, eid, current=None, passwd=None, squota=None, hquota=None):
        """ Modify an exported file system

        Args:
            eid: the export id to modify
            current: current password (only need if passwd)
            passwd: password to set if None no modification is done
            squota: soft quota to set if None no modification is done
            hquota: hard quota to set if None no modification is done
        """
        enode = self._get_exportd_node()
        econfig = enode.get_configurations(Role.EXPORTD)

        print squota

        if econfig is None:
            raise Exception("Exportd node is off line.")

        if eid not in econfig[Role.EXPORTD].exports.keys():
            raise Exception("Unknown export: %d." % eid)

        # validate quotas
        if squota is not None:
            if squota:
                if not squota[0].isdigit():
                    raise Exception("Invalid squota format: %s." % squota)

                for c in squota:
                    if not c.isdigit():
                        if c not in ['K', 'M', 'G']:
                            raise Exception("Invalid squota format: %s." % squota)
                        else:
                            break
            econfig[Role.EXPORTD].exports[eid].squota = squota

        if hquota is not None:
            if hquota:
                if not hquota[0].isdigit():
                    raise Exception("Invalid hquota format: %s." % hquota)

                for c in hquota:
                    if not c.isdigit():
                        if c not in ['K', 'M', 'G']:
                            raise Exception("Invalid hquota format: %s." % squota)
                        else:
                            break
            econfig[Role.EXPORTD].exports[eid].hquota = hquota

        # compute md5
        if passwd is not None:
            if econfig[Role.EXPORTD].exports[eid].md5:
                if current is None:
                    raise "Current password needed."

                # check current passwd
                with open('/dev/null', 'w') as devnull:
                        if econfig[Role.EXPORTD].exports[eid].md5 != subprocess.check_output(['md5pass', current, 'rozofs'],
                                       stderr=devnull)[11:].rstrip():
                            raise "Permission denied."

            if passwd:
                with open('/dev/null', 'w') as devnull:
                    econfig[Role.EXPORTD].exports[eid].md5 = subprocess.check_output(['md5pass', passwd, 'rozofs'],
                                   stderr=devnull)[11:].rstrip()
            else:
                econfig[Role.EXPORTD].exports[eid].md5 = ""

        # update this export
        enode.set_configurations(econfig)

    def remove_export(self, eids=None, force=False):
        """ Remove exported file systems

        To not keep unused data stored, exports containing files will not be remove
        unless force is set to True.

        Args:
            eids: export ids to remove
            force: force removing of non empty exportd
        """

        enode = self._get_exportd_node()
        econfig = enode.get_configurations(Role.EXPORTD)

        if econfig is None:
            raise Exception("Exportd node is off line.")

        if eids is None:
            eids = econfig[Role.EXPORTD].exports.keys()

        # unmount if needed
        self.umount_export(eids)

        # sanity check
        for eid, estat in econfig[Role.EXPORTD].stats.estats.items():
            for eeid in eids:
                if eeid not in econfig[Role.EXPORTD].exports.keys():
                    raise Exception("Unknown export: %d." % eid)
                if eid == eeid and estat.files != 0 and not force:
                    raise Exception("Can't remove non empty export (use  force=True)")


        # delete these exports from exportd configuration
        for eid in eids:
            econfig[Role.EXPORTD].exports.pop(eid)

        enode.set_configurations(econfig)

    def mount_export(self, eids=None, hosts=None, options=None):
        """ Mount an exported file system

        Only kown (managed by this platform) hosts could mount a file system.

        Args:
            eids: the export ids to mount if None all exports are mount
            hosts: target hosts to mount on if None all export will be mount on
                   all nodes
            options: mount options to use
        """

        enode = self._get_exportd_node()
        econfig = enode.get_configurations(Role.EXPORTD)

        if econfig is None:
            raise Exception("Exportd node is off line.")

        if hosts is None:
            hosts = self._nodes.keys()

        if eids is None:
            eids = econfig[Role.EXPORTD].exports.keys()


        # sanity check
        for h in hosts:
            if h not in self._nodes:
                raise Exception("Unknown host: %s." % h)
        for eid in eids:
            if eid not in econfig[Role.EXPORTD].exports.keys():
                raise Exception("Unknown export: %d." % eid)

        # mount
        for h in hosts:
            node = self._nodes[h]
            # may be overkill
            node.set_roles(node.get_roles() | Role.ROZOFSMOUNT)
            rconfigs = node.get_configurations(Role.ROZOFSMOUNT)

            for eid in eids:
                expconfig = econfig[Role.EXPORTD].exports[eid]
                rconfig = RozofsMountConfig(self._hostname , expconfig.root, -1, options)
                # check duplicates
                if rconfig not in rconfigs:
                    rconfigs[Role.ROZOFSMOUNT].append(rconfig)

            node.set_configurations(rconfigs)

    def umount_export(self, eids=None, hosts=None):
        """ Umount an exported file system

        Only kown (managed by this platform) hosts could umount a file system.

        Args:
            eids: the export ids to mount if None all exports are umount
            hosts: target hosts to mount on if None all export will be mount on
                   all nodes
        """

        if hosts is None:
            hosts = self._nodes.keys()

        enode = self._get_exportd_node()
        econfig = enode.get_configurations(Role.EXPORTD)

        if econfig is None:
            raise Exception("Exportd node is off line.")

        if eids is None:
            eids = econfig[Role.EXPORTD].exports.keys()

        # sanity check
        for h in hosts:
            if h not in self._nodes:
                raise Exception("Unknown host: %s." % h)
        for eid in eids:
            if eid not in econfig[Role.EXPORTD].exports.keys():
                raise Exception("Unknown export: %d." % eid)

        # umount
        for h in hosts:
            node = self._nodes[h]
            if node.has_one_of_roles(Role.ROZOFSMOUNT):
                rconfigs = node.get_configurations(Role.ROZOFSMOUNT)

                for eid in eids:
                    expconfig = econfig[Role.EXPORTD].exports[eid]
                    rconfig = RozofsMountConfig(self._hostname , expconfig.root, -1, None)
                    if rconfig in rconfigs[Role.ROZOFSMOUNT]:
                        rconfigs[Role.ROZOFSMOUNT].remove(rconfig)

                node.set_configurations(rconfigs)
                if not rconfigs:
                    node.set_roles(node.get_roles ^ Role.ROZOFSMOUNT)
