"""Small semantic checks for the portable evidence reader."""
import io
from pathlib import Path
import tarfile
import tempfile
import unittest
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


if __name__ == '__main__':
    unittest.main()
