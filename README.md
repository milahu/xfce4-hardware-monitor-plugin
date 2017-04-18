XFCE4 Hardware Monitor Downloads
================================

This branch contains Debian archives (<v1.4.6, made by [checkinstall](http://asic-linux.com.mx/~izto/checkinstall/)) targetted at the Devuan Testing environment I use locally (but should work with Debian
Testing).

Please report any problems on the [issue tracker](https://bugzilla.xfce.org/buglist.cgi?component=General&list_id=19900&product=Xfce4-hardware-monitor-plugin).

To download a file, click on the filename, then click View Raw. **DO NOT**
right click on a link, as this will save the web page, not the file.

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