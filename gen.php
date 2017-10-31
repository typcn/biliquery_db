<?php
for ($x=intval($argv[1]); $x<intval($argv[2]); $x++) {
  echo hex2bin(hash('crc32b',$x));
}
