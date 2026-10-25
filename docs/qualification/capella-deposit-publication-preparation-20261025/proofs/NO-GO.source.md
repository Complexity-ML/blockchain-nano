The unexecuted provider draft SHA7096ff3624ef65d7a198a318c35f4415d644f3840a66d817fbde4d7b9338de35
is retained as SOURCE_NO_GO. Its TLS stack rejected same-thread cycles, but
foreign nested calls still took blocking mutexes: two active callbacks on two
threads could wait A→B/B→A forever. The replacement uses try_lock for a foreign
nested gate, and refuses nested close contention/retirement before waiting or
poisoning an open provider's closing flag. Normal external close still joins.
No compiler, runtime or test result exists for this rejected source.
