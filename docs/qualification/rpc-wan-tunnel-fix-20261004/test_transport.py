import unittest
from pinned_transport import plan,PIN,HOST,TARGET
class Transport(unittest.TestCase):
 def test_ssh(self):self.assertEqual(plan('ssh',['-o','ConnectTimeout=8',TARGET,'printf ok']),['/usr/bin/ssh',*PIN,'-o','ConnectTimeout=8',HOST,'printf ok'])
 def test_tunnel(self):
  p=plan('ssh',['-N','-L','127.0.0.1:30307:127.0.0.1:30307',TARGET]);self.assertEqual(p[-1],HOST);self.assertIn('-N',p)
 def test_scp_push(self):self.assertEqual(plan('scp',['-q','-r','/tmp/config',TARGET+':/home/ubuntu/config'])[-1],'ubuntu@[2001:41d0:404:200::9c34]:/home/ubuntu/config')
 def test_scp_pull(self):self.assertEqual(plan('scp',['-q',TARGET+':/home/ubuntu/result','/tmp/result'])[-2],'ubuntu@[2001:41d0:404:200::9c34]:/home/ubuntu/result')
 def test_spaces_not_shell_parsed(self):self.assertEqual(plan('scp',['/tmp/a b',TARGET+':/home/ubuntu/result'])[-2],'/tmp/a b')
 def test_command_preserved(self):
  command="sudo -n docker exec node sh -c 'printf %s data'";self.assertEqual(plan('ssh',[TARGET,command])[-1],command)
 def test_unknown_host(self):
  with self.assertRaises(ValueError):plan('ssh',['ubuntu@1.2.3.4','true'])
 def test_duplicate_host(self):
  with self.assertRaises(ValueError):plan('ssh',[TARGET,TARGET])
 def test_pin_overrides(self):
  for value in ['StrictHostKeyChecking=no','HostKeyAlias=evil','UserKnownHostsFile=/dev/null','GlobalKnownHostsFile=/dev/null','ProxyCommand=evil','ProxyJump=evil']:
   with self.subTest(value=value):
    for args in [['-o',value,TARGET],['-o'+value,TARGET]]:
     with self.assertRaises(ValueError):plan('ssh',args)
 def test_config_override(self):
  for option in ['-F','-F/tmp/config','-J','-Jother']:
   with self.subTest(option=option):
    with self.assertRaises(ValueError):plan('ssh',[option,TARGET])
 def test_missing_endpoint(self):
  with self.assertRaises(ValueError):plan('scp',['-q','/tmp/a','/tmp/b'])
 def test_wrong_tool(self):
  with self.assertRaises(ValueError):plan('curl',[TARGET])
 def test_local_ipv4_family_available(self):
  p=plan('ssh',['-N','-L','127.0.0.1:30307:127.0.0.1:30307',TARGET])
  self.assertNotIn('-6',p);self.assertNotIn('-4',p);self.assertIn('AddressFamily=any',p)
  self.assertEqual(p[-1],HOST)
 def test_family_override_closed(self):
  for option in ['-4','-6','-oAddressFamily=inet','-oAddressFamily=inet6']:
   with self.subTest(option=option):
    with self.assertRaises(ValueError):plan('ssh',[option,TARGET])
 def test_separate_family_override_closed(self):
  with self.assertRaises(ValueError):plan('ssh',['-o','AddressFamily=inet6',TARGET])
if __name__=='__main__':unittest.main(verbosity=2)
