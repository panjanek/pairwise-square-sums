"""Small semantic checks for the portable evidence reader."""
import io
from pathlib import Path
import tarfile
import tempfile
import unittest
import compact
import verify


class VerificationTests(unittest.TestCase):
    def test_known_five_set(self):
        line = b'5: [763442,177458,148583,28658,7442] S=362141\n'
        five = set()
        count, _, _ = verify.scan(io.BytesIO(line+line), 1, 10**7, five, {})
        self.assertEqual(count['five'], 2)
        self.assertEqual(len(five), 1)

    def test_invalid_member_or_sum_is_rejected(self):
        for line in (b'5: [763443,177458,148583,28658,7442] S=362141\n',
                     b'5: [763442,177458,148583,28658,7442] S=362142\n'):
            with self.assertRaises(ValueError):
                verify.scan(io.BytesIO(line), 1, 10**7, set(), {})

    def test_half_open_interval(self):
        line = b'5: [763442,177458,148583,28658,7442] S=362141\n'
        with self.assertRaises(ValueError):
            verify.scan(io.BytesIO(line), 1, 362141, set(), {})

    def test_three_extensions_make_three_six_member_configurations(self):
        line = (b'M: [28860026400,17124487200,4431625200,162503200] '
                b'ext=[4981255200,2553991200,1674390681] S=50578642000\n')
        near = {}
        _, _, size = verify.scan(io.BytesIO(line), 1, 10**12, set(), near)
        self.assertEqual(size, 3)
        self.assertEqual(len(near), 3)
        self.assertTrue(all(len(v) == 6 for v in near))

    def test_archive_path_traversal_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); archive = root/'bad.tar.gz'; target = root/'target'
            target.mkdir()
            with tarfile.open(archive, 'w:gz') as tar:
                item = tarfile.TarInfo('../outside'); item.size = 1
                tar.addfile(item, io.BytesIO(b'x'))
            with self.assertRaises(ValueError):
                verify.unpack(archive, target)
            self.assertFalse((root/'outside').exists())

    def test_checkpoint_requires_every_counter_and_done(self):
        totals = dict.fromkeys(compact.COUNTERS, 0)
        totals['S_done'] = 20
        block = {'lo':100, 'hi':120, 'totals':totals}
        manifest = {'threads':2, 'chunk_log2':20}
        checkpoint = ('s4ckpt v1\n100 120 2 1048576\n'
                      '0 110 110 10 0 0 0 0 0 0 0 0 0\n'
                      '1 120 120 10 0 0 0 0 0 0 0 0 0\nDONE\n')
        self.assertEqual(len(compact.check_checkpoint(verify, checkpoint, block, manifest)), 2)
        for broken in (checkpoint.replace('DONE\n', ''),
                       checkpoint.replace('1 120 120', '1 119 120'),
                       checkpoint.replace('0 110 110 10 0', '0 110 110 10 1')):
            with self.assertRaises(ValueError):
                compact.check_checkpoint(verify, broken, block, manifest)

    def test_predecessor_nested_name_and_hash_are_checked(self):
        with tempfile.TemporaryDirectory() as tmp:
            previous = Path(tmp)/'previous'; previous.mkdir()
            path = previous/'manifest.json'; path.write_text('{}')
            predecessor = {'directory':'/old/private/runs/previous/exhaustive',
                           'frontier':120, 'files':{'manifest.json':verify.digest(path)}}
            compact.check_predecessor(verify, predecessor, previous, 'previous', 120, {'start':120})
            with self.assertRaises(ValueError):
                compact.check_predecessor(verify, predecessor, previous, 'previous', 120, {'start':121})
            path.write_text('{"changed":true}')
            with self.assertRaises(ValueError):
                compact.check_predecessor(verify, predecessor, previous, 'previous', 120, {'start':120})


if __name__ == '__main__':
    unittest.main()
