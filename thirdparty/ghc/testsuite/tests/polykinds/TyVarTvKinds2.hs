{-# LANGUAGE PolyKinds #-}

module TyVarTvKinds2 where

data SameKind :: k -> k -> *

data Q (a :: k1) (b :: k2) c = MkQ (SameKind a b)
