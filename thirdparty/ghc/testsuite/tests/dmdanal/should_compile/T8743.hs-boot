{-# LANGUAGE Haskell2010 #-}
module T8743 where
class ToRow a
instance ToRow (Maybe a)
