XFCE4 Hardware Monitor Downloads
================================

This branch contains Debian archives:

<v1.6: Historical interest only, built for Devuan Testing (compatible with
Debian Testing) at the time of the relevant release  (<v1.4.6, made by
[checkinstall](http://asic-linux.com.mx/~izto/checkinstall/)).
v1.6: As of 14.01.18, built for Debian Stable (Stretch) and Debian Testing
(Buster) in Debian VMs.

Please report any problems on the [issue tracker](https://bugzilla.xfce.org/buglist.cgi?component=General&list_id=19900&product=Xfce4-hardware-monitor-plugin).

To download a file, use the 'plain' link on the far right - anything else will
treat it as a binary file to view in the cgit system.

From v1.4.6 these archives are signed by my GPG key - to verify:

1. Install dpkg-sig: sudo aptitude install dpkg-sig gpg
2. Fetch my public key: gpg --keyserver keys.gnupg.net --recv-keys 0xFDC2F38F
3. Confirm that the fingerprint of the key matches (so that you can trust my key): E760 95EC DACD 5DEC 7653 A996 17D2 3C7D FDC2 F38F
4. Verify the debian archive: dpkg-sig --verify *.deb

If you haven't imported my key, you'll get this:

===============================

UNKNOWNSIG _gpgbuilder FDC2F38F

===============================

Otherwise:

=======================================================================

GOODSIG _gpgbuilder E76095ECDACD5DEC7653A99617D23C7DFDC2F38F 1407865602

=======================================================================
